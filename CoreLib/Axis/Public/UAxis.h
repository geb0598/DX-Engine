#pragma once

#include <d3d11.h>

#include <wrl.h>

#include "Containers/Containers.h"
#include "Types/Types.h"
#include "Mesh/Public/Vertex.h"

#define AXIS_LENGTH 10000.0f

class UAxisDrawer
{
public:
	~UAxisDrawer() = default;

	UAxisDrawer(ID3D11Device* Device, const TArray<FVertex>& VertexArray);

	UAxisDrawer(const UAxisDrawer&) = delete;
	UAxisDrawer(UAxisDrawer&&) noexcept = default;

	UAxisDrawer& operator=(const UAxisDrawer&) = delete;
	UAxisDrawer& operator=(UAxisDrawer&&) noexcept = default;

	void Render(ID3D11DeviceContext* DeviceContext);
private:
	ID3D11Buffer* VertexBuffer;

	const UINT VertexCount;
	const UINT Stride;
};