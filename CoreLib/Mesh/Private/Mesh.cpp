#include <d3d11.h>

#include "Types/Types.h"

#include "Mesh/Public/Mesh.h"

UMesh::UMesh(ID3D11Device* Device, const TArray<FVertex>& VertexArray)
	: Vertices(VertexArray), VertexCount(VertexArray.size()), Stride(sizeof(FVertex))
{
	D3D11_BUFFER_DESC VertexBufferDesc = {};
	VertexBufferDesc.ByteWidth = static_cast<UINT>(VertexCount * Stride);
	VertexBufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
	VertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	VertexBufferDesc.CPUAccessFlags = 0;
	VertexBufferDesc.MiscFlags = 0;
	VertexBufferDesc.StructureByteStride = 0;

	D3D11_SUBRESOURCE_DATA VertexBufferSRD = { VertexArray.data() };

	Device->CreateBuffer(&VertexBufferDesc, &VertexBufferSRD, VertexBuffer.ReleaseAndGetAddressOf());
}

void UMesh::Bind(ID3D11DeviceContext* DeviceContext) const
{
	UINT Offset = 0;
	DeviceContext->IASetVertexBuffers(0, 1, VertexBuffer.GetAddressOf(), &Stride, &Offset);

	DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

UINT UMesh::GetVertexCount() const
{
	return VertexCount;
}

UINT UMesh::GetStride() const
{
	return Stride;
}

const TArray<FVertex>& UMesh::GetVertices() const
{
	return Vertices;
}