#pragma once

#include <d3d11.h>

#include "Math/Public/Vector.h"

struct FVertexSimple
{
	float x, y, z;		// Position
	float r, g, b, a;	// Color
};

struct FVertex
{
	FVector Position;
	FVector4 Color;

	FVertex operator() (FVertexSimple VertexSimple)
	{
		FVertex Vertex = {};
		Vertex.Position = { VertexSimple.x, VertexSimple.y, VertexSimple.z };
		Vertex.Color = { VertexSimple.r, VertexSimple.g, VertexSimple.b, VertexSimple.a };
		return Vertex;
	}

	static const D3D11_INPUT_ELEMENT_DESC InputLayoutDesc[2];
};

inline const D3D11_INPUT_ELEMENT_DESC FVertex::InputLayoutDesc[2] =
{
	{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA},
	{"COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA}
};
