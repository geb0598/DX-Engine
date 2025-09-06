#pragma once

#include <d3d11.h>

#include "Math/Public/Vector.h"

struct FVertex
{
	FVector Position;
	FVector4 Color;

	static const D3D11_INPUT_ELEMENT_DESC InputLayout[2];
};

inline const D3D11_INPUT_ELEMENT_DESC FVertex::InputLayout[2] =
{
	{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA},
	{"COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA}
};
