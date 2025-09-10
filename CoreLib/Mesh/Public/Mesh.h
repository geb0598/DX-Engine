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

	UMesh(ID3D11Device* Device, TArray<FVertex> VertexArray);

	UMesh(const UMesh&) = delete;
	UMesh(UMesh&&) noexcept = default;

	UMesh& operator=(const UMesh&) = delete;
	UMesh& operator=(UMesh&&) noexcept = default;

	void Bind(ID3D11DeviceContext* DeviceContext) const;

	const TArray<FVertex>& GetVertexArray() const;

	UINT GetVertexCount() const;
	UINT GetStride() const;

	// [추가] 정점 데이터에 접근하기 위한 Getter 함수
	const TArray<FVertex>& GetVertices() const;

private:
	Microsoft::WRL::ComPtr<ID3D11Buffer> VertexBuffer;

	TArray<FVertex> VertexArray;

	const UINT VertexCount;
	const UINT Stride;

	// [추가] 원본 정점 데이터를 멤버로 저장
	TArray<FVertex> Vertices;
};
