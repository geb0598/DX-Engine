#include "../Public/UAxis.h"

UAxisDrawer::UAxisDrawer(ID3D11Device* Device, const TArray<FVertex>& VertexArray)
	: VertexCount(VertexArray.size()), Stride(sizeof(FVertex))
{
	D3D11_BUFFER_DESC VertexBufferDesc = {};
	VertexBufferDesc.ByteWidth = static_cast<UINT>(VertexCount * Stride);
	VertexBufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
	VertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	VertexBufferDesc.CPUAccessFlags = 0;
	VertexBufferDesc.MiscFlags = 0;
	VertexBufferDesc.StructureByteStride = 0;

	D3D11_SUBRESOURCE_DATA VertexBufferSRD = { VertexArray.data() };

	Device->CreateBuffer(&VertexBufferDesc, &VertexBufferSRD, &VertexBuffer);
}

void UAxisDrawer::Render(ID3D11DeviceContext* DeviceContext)
{
	UINT Offset = 0;
	DeviceContext->IASetVertexBuffers(0, 1, &VertexBuffer, &Stride, &Offset);
	DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);

	DeviceContext->Draw(VertexCount, 0);
}