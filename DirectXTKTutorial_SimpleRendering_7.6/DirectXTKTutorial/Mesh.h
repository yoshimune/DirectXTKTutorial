#pragma once

#include "pch.h"
class Mesh
{

public:
	Mesh(D3D_PRIMITIVE_TOPOLOGY topology, 
		DirectX::VertexPositionColor vertices[],
		int count,
		DirectX::SimpleMath::Vector3 position, 
		DirectX::SimpleMath::Vector3 scale);
	~Mesh();

	const D3D_PRIMITIVE_TOPOLOGY GetTopology() const;
	const int GetCount() const;
	const DirectX::VertexPositionColor* GetVertices(std::unique_ptr<DirectX::VertexPositionColor[]>& result);
	void SetPosition(DirectX::SimpleMath::Vector3 position);

private:
	const D3D_PRIMITIVE_TOPOLOGY topology;
	std::unique_ptr<DirectX::VertexPositionColor[]> vertices;
	int count;
	DirectX::SimpleMath::Vector3 position;
	DirectX::SimpleMath::Vector3 scale;

	DirectX::SimpleMath::Vector3 ApplyPosition(DirectX::SimpleMath::Vector3 origin) const;
	DirectX::SimpleMath::Vector3 ApplyScale(DirectX::SimpleMath::Vector3 origin) const;
};
