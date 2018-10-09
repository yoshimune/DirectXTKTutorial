//
// Game.cpp
//

#include "pch.h"
#include "Game.h"

extern void ExitGame();

using namespace DirectX;
using namespace DirectX::SimpleMath;

using Microsoft::WRL::ComPtr;

namespace
{
	struct VS_BLOOM_PARAMETERS
	{
		float bloomThreshold;
		float blurAmount;
		float bloomIntensity;
		float baseIntensity;
		float bloomSaturation;
		float baseSaturation;
		uint8_t na[8];
	};

	static_assert(!(sizeof(VS_BLOOM_PARAMETERS) % 16), "VS_BLOOM_PARAMETERS needs to be 16 bytes aligned.");

	struct VS_BLUR_PARAMETERS
	{
		static const size_t SAMPLE_COUNT = 15;

		XMFLOAT4 sampleOffsets[SAMPLE_COUNT];
		XMFLOAT4 sampleWeights[SAMPLE_COUNT];

		void SetBlurEffectParameters(float dx, float dy, const VS_BLOOM_PARAMETERS& params)
		{
			sampleWeights[0].x = ComputeGaussian(0, params.blurAmount);
			sampleOffsets[0].x = sampleOffsets[0].y = 0.f;

			float totalWeights = sampleWeights[0].x;

			// Add pairs of additional sample taps, positioned
			// along a line in both directions from the center
			for (size_t i = 0; i < SAMPLE_COUNT / 2; i++)
			{
				// Store weights for the positive and regative taps
				float weight = ComputeGaussian(float(i + 1.f), params.blurAmount);

				sampleWeights[i * 2 + 1].x = weight;
				sampleWeights[i * 2 + 2].x = weight;

				totalWeights += weight * 2;

				// To get the maximum amount of blurring from a limited number of pixel shader samples,
				// we take advantage of the bilinear filtering hardware inside the texture fetch uint.
				// If we position our texture coordinates exactly halfway between two texels, 
				// the filtering uint will average them for us, giving two samples for the price of one.
				// This allows us to step in uints of two texels per sample, rather than just one at a time.
				// The 1.5 offset kicks things off by positioning us nicely in between two texels.
				float sampleOffset = float(i) * 2.f + 1.5f;

				Vector2 delta = Vector2(dx, dy) * sampleOffset;

				//Store texture coordinate offsets for the positive and negative taps.
				sampleOffsets[i * 2 + 1].x = delta.x;
				sampleOffsets[i * 2 + 1].y = delta.y;
				sampleOffsets[i * 2 + 2].x = -delta.x;
				sampleOffsets[i * 2 + 2].y = -delta.y;
			}

			for (size_t i = 0;i < SAMPLE_COUNT;i++)
			{
				sampleWeights[i].x /= totalWeights;
			}
		}

	private:
		float ComputeGaussian(float n, float theta)
		{
			return (float)((1.0 / sqrtf(2 * XM_PI * theta)) * expf(-(n * n) / (2 * theta * theta)));
		}
	};

	static_assert(!(sizeof(VS_BLUR_PARAMETERS) % 16), "VS_BLUR_PARAMETERS needs to be 16 bytes aligned");

	enum BloomPresets
	{
		Default = 0,
		Soft,
		Desaturated,
		Saturated,
		Blurry,
		Subtle,
		None
	};

	BloomPresets g_Bloom = Default;

	static const VS_BLOOM_PARAMETERS g_BloomPresets[] =
	{
		// Thresh	Blur	Bloom	Base	BloomSat	BaseSat
		{ 0.25f,	4,		1.25f,	1,		1,			1}, // Default
		{ 0,		3,		1,		1,		1,			1}, // Soft
		{ 0.5f,		8,		2,		1,		0,			1}, // Desaturated
		{ 0.25f,	4,		2,		1,		2,			0}, // Saturated
		{ 0,		2,		1,		0.1f,	1,			1}, // Blurry
		{ 0.5f,		2,		1,		1,		1,			1}, // Subtle
		{ 0.25f,	4,		1.25f,	1,		1,			1}, // None
	};
}

Game::Game() noexcept :
    m_window(nullptr),
    m_outputWidth(800),
    m_outputHeight(600),
    m_featureLevel(D3D_FEATURE_LEVEL_9_1)
{
}

// Initialize the Direct3D resources required to run.
void Game::Initialize(HWND window, int width, int height)
{
    m_window = window;
    m_outputWidth = std::max(width, 1);
    m_outputHeight = std::max(height, 1);

    CreateDevice();

    CreateResources();

    // TODO: Change the timer settings if you want something other than the default variable timestep mode.
    // e.g. for 60 FPS fixed timestep update logic, call:
    /*
    m_timer.SetFixedTimeStep(true);
    m_timer.SetTargetElapsedSeconds(1.0 / 60);
    */
}

// Executes the basic game loop.
void Game::Tick()
{
    m_timer.Tick([&]()
    {
        Update(m_timer);
    });

    Render();
}

// Updates the world.
void Game::Update(DX::StepTimer const& timer)
{
    float elapsedTime = float(timer.GetElapsedSeconds());

    // TODO: Add your game logic here.
	float totalTime = static_cast<float>(timer.GetTotalSeconds());

	m_world = Matrix::CreateRotationZ(totalTime / 2.f) * Matrix::CreateRotationY(totalTime) * Matrix::CreateRotationX(totalTime * 2.f);
}

// Draws the scene.
void Game::Render()
{
    // Don't try to render anything before the first Update.
    if (m_timer.GetFrameCount() == 0)
    {
        return;
    }

    Clear();

    // TODO: Add your rendering code here.
	m_spriteBatch->Begin();
	m_spriteBatch->Draw(m_background.Get(), m_fullscreenRect);
	m_spriteBatch->End();

	m_shape->Draw(m_world, m_view, m_projction);

    Present();
}

// Helper method to clear the back buffers.
void Game::Clear()
{
    // Clear the views.
    //m_d3dContext->ClearRenderTargetView(m_renderTargetView.Get(), Colors::CornflowerBlue);
	//m_d3dContext->ClearDepthStencilView(m_depthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
	m_d3dContext->ClearDepthStencilView(m_depthStencilView.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);

	m_d3dContext->OMSetRenderTargets(1, m_renderTargetView.GetAddressOf(), m_depthStencilView.Get());

    // Set the viewport.
    CD3D11_VIEWPORT viewport(0.0f, 0.0f, static_cast<float>(m_outputWidth), static_cast<float>(m_outputHeight));
    m_d3dContext->RSSetViewports(1, &viewport);
}

// Presents the back buffer contents to the screen.
void Game::Present()
{
    // The first argument instructs DXGI to block until VSync, putting the application
    // to sleep until the next VSync. This ensures we don't waste any cycles rendering
    // frames that will never be displayed to the screen.
    HRESULT hr = m_swapChain->Present(1, 0);

    // If the device was reset we must completely reinitialize the renderer.
    if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
    {
        OnDeviceLost();
    }
    else
    {
        DX::ThrowIfFailed(hr);
    }
}

// Message handlers
void Game::OnActivated()
{
    // TODO: Game is becoming active window.
}

void Game::OnDeactivated()
{
    // TODO: Game is becoming background window.
}

void Game::OnSuspending()
{
    // TODO: Game is being power-suspended (or minimized).
}

void Game::OnResuming()
{
    m_timer.ResetElapsedTime();

    // TODO: Game is being power-resumed (or returning from minimize).
}

void Game::OnWindowSizeChanged(int width, int height)
{
    m_outputWidth = std::max(width, 1);
    m_outputHeight = std::max(height, 1);

    CreateResources();

    // TODO: Game window is being resized.
}

// Properties
void Game::GetDefaultSize(int& width, int& height) const
{
    // TODO: Change to desired default window size (note minimum size is 320x200).
    width = 800;
    height = 600;
}

// These are the resources that depend on the device.
void Game::CreateDevice()
{
    UINT creationFlags = 0;

#ifdef _DEBUG
    creationFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    static const D3D_FEATURE_LEVEL featureLevels [] =
    {
        // TODO: Modify for supported Direct3D feature levels
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0,
        D3D_FEATURE_LEVEL_9_3,
        D3D_FEATURE_LEVEL_9_2,
        D3D_FEATURE_LEVEL_9_1,
    };

    // Create the DX11 API device object, and get a corresponding context.
    ComPtr<ID3D11Device> device;
    ComPtr<ID3D11DeviceContext> context;
    DX::ThrowIfFailed(D3D11CreateDevice(
        nullptr,                            // specify nullptr to use the default adapter
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        creationFlags,
        featureLevels,
        _countof(featureLevels),
        D3D11_SDK_VERSION,
        device.ReleaseAndGetAddressOf(),    // returns the Direct3D device created
        &m_featureLevel,                    // returns feature level of device created
        context.ReleaseAndGetAddressOf()    // returns the device immediate context
        ));

#ifndef NDEBUG
    ComPtr<ID3D11Debug> d3dDebug;
    if (SUCCEEDED(device.As(&d3dDebug)))
    {
        ComPtr<ID3D11InfoQueue> d3dInfoQueue;
        if (SUCCEEDED(d3dDebug.As(&d3dInfoQueue)))
        {
#ifdef _DEBUG
            d3dInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_CORRUPTION, true);
            d3dInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_ERROR, true);
#endif
            D3D11_MESSAGE_ID hide [] =
            {
                D3D11_MESSAGE_ID_SETPRIVATEDATA_CHANGINGPARAMS,
                // TODO: Add more message IDs here as needed.
            };
            D3D11_INFO_QUEUE_FILTER filter = {};
            filter.DenyList.NumIDs = _countof(hide);
            filter.DenyList.pIDList = hide;
            d3dInfoQueue->AddStorageFilterEntries(&filter);
        }
    }
#endif

    DX::ThrowIfFailed(device.As(&m_d3dDevice));
    DX::ThrowIfFailed(context.As(&m_d3dContext));

    // TODO: Initialize device dependent objects here (independent of window size).
	DX::ThrowIfFailed(CreateWICTextureFromFile(m_d3dDevice.Get(), L"sunset.jpg", nullptr, m_background.ReleaseAndGetAddressOf()));

	m_states = std::make_unique<CommonStates>(m_d3dDevice.Get());
	m_spriteBatch = std::make_unique <SpriteBatch>(m_d3dContext.Get());
	m_shape = GeometricPrimitive::CreateTorus(m_d3dContext.Get());

	m_view = Matrix::CreateLookAt(Vector3(0.f, 3.f, -3.f), Vector3::Zero, Vector3::UnitY);

	auto blob = DX::ReadData(L"BloomExtract.cso");
	DX::ThrowIfFailed(m_d3dDevice->CreatePixelShader(blob.data(), blob.size(), nullptr, m_bloomExtractPS.ReleaseAndGetAddressOf()));

	blob = DX::ReadData(L"BloomCombine.cso");
	DX::ThrowIfFailed(m_d3dDevice->CreatePixelShader(blob.data(), blob.size(), nullptr, m_bloomCombinePS.ReleaseAndGetAddressOf()));

	blob = DX::ReadData(L"GaussianBlur.cso");
	DX::ThrowIfFailed(m_d3dDevice->CreatePixelShader(blob.data(), blob.size(), nullptr, m_gaussianBlurPS.ReleaseAndGetAddressOf()));

	{
		CD3D11_BUFFER_DESC cbDesc(sizeof(VS_BLOOM_PARAMETERS), D3D11_BIND_CONSTANT_BUFFER);
		D3D11_SUBRESOURCE_DATA initData;
		initData.pSysMem = &g_BloomPresets[g_Bloom];
		initData.SysMemPitch = sizeof(VS_BLOOM_PARAMETERS);
		DX::ThrowIfFailed(m_d3dDevice->CreateBuffer(&cbDesc, &initData, m_bloomParams.ReleaseAndGetAddressOf()));
	}

	{
		CD3D11_BUFFER_DESC cbDesc(sizeof(VS_BLUR_PARAMETERS), D3D11_BIND_CONSTANT_BUFFER);
		DX::ThrowIfFailed(m_d3dDevice->CreateBuffer(&cbDesc, nullptr, m_blurParamsWidth.ReleaseAndGetAddressOf()));
		DX::ThrowIfFailed(m_d3dDevice->CreateBuffer(&cbDesc, nullptr, m_blurParamsHeight.ReleaseAndGetAddressOf()));
	}
}

// Allocate all memory resources that change on a window SizeChanged event.
void Game::CreateResources()
{
    // Clear the previous window size specific context.
    ID3D11RenderTargetView* nullViews [] = { nullptr };
    m_d3dContext->OMSetRenderTargets(_countof(nullViews), nullViews, nullptr);
    m_renderTargetView.Reset();
    m_depthStencilView.Reset();
    m_d3dContext->Flush();

    UINT backBufferWidth = static_cast<UINT>(m_outputWidth);
    UINT backBufferHeight = static_cast<UINT>(m_outputHeight);
    DXGI_FORMAT backBufferFormat = DXGI_FORMAT_B8G8R8A8_UNORM;
    DXGI_FORMAT depthBufferFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
    UINT backBufferCount = 2;

    // If the swap chain already exists, resize it, otherwise create one.
    if (m_swapChain)
    {
        HRESULT hr = m_swapChain->ResizeBuffers(backBufferCount, backBufferWidth, backBufferHeight, backBufferFormat, 0);

        if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
        {
            // If the device was removed for any reason, a new device and swap chain will need to be created.
            OnDeviceLost();

            // Everything is set up now. Do not continue execution of this method. OnDeviceLost will reenter this method 
            // and correctly set up the new device.
            return;
        }
        else
        {
            DX::ThrowIfFailed(hr);
        }
    }
    else
    {
        // First, retrieve the underlying DXGI Device from the D3D Device.
        ComPtr<IDXGIDevice1> dxgiDevice;
        DX::ThrowIfFailed(m_d3dDevice.As(&dxgiDevice));

        // Identify the physical adapter (GPU or card) this device is running on.
        ComPtr<IDXGIAdapter> dxgiAdapter;
        DX::ThrowIfFailed(dxgiDevice->GetAdapter(dxgiAdapter.GetAddressOf()));

        // And obtain the factory object that created it.
        ComPtr<IDXGIFactory2> dxgiFactory;
        DX::ThrowIfFailed(dxgiAdapter->GetParent(IID_PPV_ARGS(dxgiFactory.GetAddressOf())));

        // Create a descriptor for the swap chain.
        DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
        swapChainDesc.Width = backBufferWidth;
        swapChainDesc.Height = backBufferHeight;
        swapChainDesc.Format = backBufferFormat;
        swapChainDesc.SampleDesc.Count = 1;
        swapChainDesc.SampleDesc.Quality = 0;
        swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChainDesc.BufferCount = backBufferCount;

        DXGI_SWAP_CHAIN_FULLSCREEN_DESC fsSwapChainDesc = {};
        fsSwapChainDesc.Windowed = TRUE;

        // Create a SwapChain from a Win32 window.
        DX::ThrowIfFailed(dxgiFactory->CreateSwapChainForHwnd(
            m_d3dDevice.Get(),
            m_window,
            &swapChainDesc,
            &fsSwapChainDesc,
            nullptr,
            m_swapChain.ReleaseAndGetAddressOf()
            ));

        // This template does not support exclusive fullscreen mode and prevents DXGI from responding to the ALT+ENTER shortcut.
        DX::ThrowIfFailed(dxgiFactory->MakeWindowAssociation(m_window, DXGI_MWA_NO_ALT_ENTER));
    }

    // Obtain the backbuffer for this window which will be the final 3D rendertarget.
    ComPtr<ID3D11Texture2D> backBuffer;
    DX::ThrowIfFailed(m_swapChain->GetBuffer(0, IID_PPV_ARGS(backBuffer.GetAddressOf())));

    // Create a view interface on the rendertarget to use on bind.
    DX::ThrowIfFailed(m_d3dDevice->CreateRenderTargetView(backBuffer.Get(), nullptr, m_renderTargetView.ReleaseAndGetAddressOf()));

    // Allocate a 2-D surface as the depth/stencil buffer and
    // create a DepthStencil view on this surface to use on bind.
    CD3D11_TEXTURE2D_DESC depthStencilDesc(depthBufferFormat, backBufferWidth, backBufferHeight, 1, 1, D3D11_BIND_DEPTH_STENCIL);

    ComPtr<ID3D11Texture2D> depthStencil;
    DX::ThrowIfFailed(m_d3dDevice->CreateTexture2D(&depthStencilDesc, nullptr, depthStencil.GetAddressOf()));

    CD3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc(D3D11_DSV_DIMENSION_TEXTURE2D);
    DX::ThrowIfFailed(m_d3dDevice->CreateDepthStencilView(depthStencil.Get(), &depthStencilViewDesc, m_depthStencilView.ReleaseAndGetAddressOf()));

    // TODO: Initialize windows-size dependent objects here.

	m_fullscreenRect.left = 0;
	m_fullscreenRect.top = 0;
	m_fullscreenRect.right = backBufferWidth;
	m_fullscreenRect.bottom = backBufferHeight;

	m_projction = Matrix::CreatePerspectiveFieldOfView(XM_PIDIV4, float(backBufferWidth) / float(backBufferHeight), 0.01f, 100.f);

	VS_BLUR_PARAMETERS blurData;
	blurData.SetBlurEffectParameters(1.f / (backBufferWidth / 2), 0, g_BloomPresets[g_Bloom]);
	m_d3dContext->UpdateSubresource(m_blurParamsWidth.Get(), 0, nullptr, &blurData, sizeof(VS_BLUR_PARAMETERS), 0);

	blurData.SetBlurEffectParameters(0, 1.f / (backBufferHeight / 2), g_BloomPresets[g_Bloom]);
	m_d3dContext->UpdateSubresource(m_blurParamsHeight.Get(), 0, nullptr, &blurData, sizeof(VS_BLUR_PARAMETERS), 0);
}

void Game::OnDeviceLost()
{
    // TODO: Add Direct3D resource cleanup here.
	m_states.reset();
	m_spriteBatch.reset();
	m_shape.reset();
	m_background.Reset();

	m_bloomExtractPS.Reset();
	m_bloomCombinePS.Reset();
	m_gaussianBlurPS.Reset();

	m_bloomParams.Reset();
	m_blurParamsWidth.Reset();
	m_blurParamsHeight.Reset();

    m_depthStencilView.Reset();
    m_renderTargetView.Reset();
    m_swapChain.Reset();
    m_d3dContext.Reset();
    m_d3dDevice.Reset();

    CreateDevice();

    CreateResources();
}