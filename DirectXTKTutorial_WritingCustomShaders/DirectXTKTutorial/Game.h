//
// Game.h
//

#pragma once

#include "StepTimer.h"


// A basic game implementation that creates a D3D11 device and
// provides a game loop.
class Game
{
public:

    Game() noexcept;

    // Initialization and management
    void Initialize(HWND window, int width, int height);

    // Basic game loop
    void Tick();

    // Messages
    void OnActivated();
    void OnDeactivated();
    void OnSuspending();
    void OnResuming();
    void OnWindowSizeChanged(int width, int height);

    // Properties
    void GetDefaultSize( int& width, int& height ) const;

private:

    void Update(DX::StepTimer const& timer);
    void Render();

	void PostProcess();

    void Clear();
    void Present();

    void CreateDevice();
    void CreateResources();

    void OnDeviceLost();

    // Device resources.
    HWND                                            m_window;
    int                                             m_outputWidth;
    int                                             m_outputHeight;

    D3D_FEATURE_LEVEL                               m_featureLevel;
    Microsoft::WRL::ComPtr<ID3D11Device1>           m_d3dDevice;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext1>    m_d3dContext;

    Microsoft::WRL::ComPtr<IDXGISwapChain1>         m_swapChain;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView>  m_renderTargetView;
    Microsoft::WRL::ComPtr<ID3D11DepthStencilView>  m_depthStencilView;

    // Rendering loop timer.
    DX::StepTimer                                   m_timer;

	std::unique_ptr<DirectX::CommonStates> m_states;
	std::unique_ptr<DirectX::SpriteBatch> m_spriteBatch;
	std::unique_ptr<DirectX::GeometricPrimitive> m_shape;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_background;
	DirectX::SimpleMath::Matrix m_world;
	DirectX::SimpleMath::Matrix m_view;
	DirectX::SimpleMath::Matrix m_projction;
	RECT m_fullscreenRect;

	Microsoft::WRL::ComPtr<ID3D11PixelShader> m_bloomExtractPS;
	Microsoft::WRL::ComPtr<ID3D11PixelShader> m_bloomCombinePS;
	Microsoft::WRL::ComPtr<ID3D11PixelShader> m_gaussianBlurPS;

	Microsoft::WRL::ComPtr<ID3D11Buffer> m_bloomParams;
	Microsoft::WRL::ComPtr<ID3D11Buffer> m_blurParamsWidth;
	Microsoft::WRL::ComPtr<ID3D11Buffer> m_blurParamsHeight;

	Microsoft::WRL::ComPtr<ID3D11Texture2D> m_backBuffer;

	Microsoft::WRL::ComPtr<ID3D11Texture2D> m_sceneTex;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_sceneSRV;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> m_sceneRT;

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_rt1SRV;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> m_rt1RT;

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_rt2SRV;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> m_rt2RT;

	RECT m_bloomRect;
};