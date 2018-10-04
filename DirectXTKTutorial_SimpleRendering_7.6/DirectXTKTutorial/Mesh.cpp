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

const VertexPositionColor* Mesh::GetVertices()
{
	//VertexPositionColor* p = *vertices.get();
	for (int i = 0; i < count; i++) {
		vertices[i].position = ApplyScale(vertices[i].position);
		vertices[i].position = ApplyPosition(vertices[i].position);
	}

	return vertices.get();
}


Vector3 Mesh::ApplyPosition(Vector3 origin) const
{
	return origin + position;
}

Vector3 Mesh::ApplyScale(Vector3 origin) const
{
	return origin * scale;
}
