#pragma once

#include <d3d11.h>

#include <wrl.h>

#include "Containers/Containers.h"
#include "Types/Types.h"
#include "Mesh/Public/Vertex.h"

class UMesh
{
public:
	~UMesh() = default;

	UMesh(ID3D11Device* Device, const TArray<FVertex>& VertexArray);

	UMesh(const UMesh&) = delete;
	UMesh(UMesh&&) noexcept = default;

	UMesh& operator=(const UMesh&) = delete;
	UMesh& operator=(UMesh&&) noexcept = default;

	void Bind(ID3D11DeviceContext* DeviceContext) const;

private:
	Microsoft::WRL::ComPtr<ID3D11Buffer> VertexBuffer;

	const UINT VertexCount;
	const UINT Stride;
};
