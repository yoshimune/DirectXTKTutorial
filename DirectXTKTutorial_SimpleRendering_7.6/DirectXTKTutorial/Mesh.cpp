#include "pch.h"
#include "Mesh.h"

using namespace DirectX;
using namespace DirectX::SimpleMath;

Mesh::Mesh(D3D_PRIMITIVE_TOPOLOGY topology,
	VertexPositionColor vertices[],
	int count,
	Vector3 position,
	Vector3 scale)
	:topology(topology)
	,count(count)
	,position(position)
	,scale(scale)
{
	this->vertices = std::make_unique<VertexPositionColor[]>(count);

	VertexPositionColor* p = vertices;
	for (int i = 0; i < count; i++) {
		this->vertices[i] = *p;
		p++;
	}
}


Mesh::~Mesh()
{
}

const D3D_PRIMITIVE_TOPOLOGY Mesh::GetTopology() const
{
	return topology;
}

const int Mesh::GetCount() const
{
	return count;
}

const VertexPositionColor* Mesh::GetVertices(std::unique_ptr<VertexPositionColor[]>& result)
{

	for (int i = 0; i < count; i++) {
		result[i].color = vertices[i].color;
		result[i].position = ApplyScale(vertices[i].position);
		result[i].position = ApplyPosition(result[i].position);
	}

	return vertices.get();
}

void Mesh::SetPosition(DirectX::SimpleMath::Vector3 position)
{
	this->position = position;
}


Vector3 Mesh::ApplyPosition(Vector3 origin) const
{
	return origin + position;
}

Vector3 Mesh::ApplyScale(Vector3 origin) const
{
	return origin * scale;
}
