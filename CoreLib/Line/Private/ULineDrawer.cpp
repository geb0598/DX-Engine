#include "../Public/ULineDrawer.h"
#include "Shader/Shader.h"
#define AXIS_LENGTH 10000.0f

FVertexSimple XAxisVertices[] =
{
	{ 0.0f, 0.0f, 0.0f,  1.0f, 0.0f, 0.0f, 1.0f },
	{ AXIS_LENGTH, 0.0f, 0.0f,  1.0f, 0.0f, 0.0f, 1.0f }
};

// green line
FVertexSimple YAxisVertices[] =
{
	{ 0.0f, 0.0f, 0.0f,  0.0f, 1.0f, 0.0f, 1.0f },
	{ 0.0f, AXIS_LENGTH, 0.0f,  0.0f, 1.0f, 0.0f, 1.0f }
};

// blue line
FVertexSimple ZAxisVertices[] =
{
	{ 0.0f, 0.0f, 0.0f,  0.0f, 0.0f, 1.0f, 1.0f },
	{ 0.0f, 0.0f, AXIS_LENGTH,  0.0f, 0.0f, 1.0f, 1.0f }
};

ULineDrawer::ULineDrawer(ID3D11Device* Device, ID3D11DeviceContext* DeviceContext)
	: Device(Device), DeviceContext(DeviceContext)
{
}

void ULineDrawer::Bind(const TArray<FVertex>& VertexArray)
{
	VertexCount = VertexArray.size();
	Stride = sizeof(FVertex);

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

void ULineDrawer::Render()
{
	UINT Offset = 0;
	DeviceContext->IASetVertexBuffers(0, 1, &VertexBuffer, &Stride, &Offset);
	DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);

	DeviceContext->Draw(VertexCount, 0);
}

void ULineDrawer::RenderXYZAxis(ID3D11Device* Device, ID3D11DeviceContext* DeviceContext)
{
	ULineDrawer LineDrawer(Device, DeviceContext);

	TArray<FVertex> XAxisArray;
	for (int i = 0; i < sizeof(XAxisVertices) / sizeof(FVertexSimple); i++)
		XAxisArray.push_back((FVertex)XAxisVertices[i]);
	LineDrawer.Bind(XAxisArray);
	LineDrawer.Render();

	TArray<FVertex> YAxisArray;
	for (int i = 0; i < sizeof(YAxisVertices) / sizeof(FVertexSimple); i++)
		YAxisArray.push_back((FVertex)YAxisVertices[i]);
	LineDrawer.Bind(YAxisArray);
	LineDrawer.Render();

	TArray<FVertex> ZAxisArray;
	for (int i = 0; i < sizeof(ZAxisVertices) / sizeof(FVertexSimple); i++)
		ZAxisArray.push_back((FVertex)ZAxisVertices[i]);
	LineDrawer.Bind(ZAxisArray);
	LineDrawer.Render();

	return;
}
