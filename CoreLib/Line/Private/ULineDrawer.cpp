#include "../Public/ULineDrawer.h"
#include "Shader/Shader.h"
#include "AssetManager/Public/AssetManager.h"
#define AXIS_LENGTH 10000.0f

// Static members initialization
ID3D11Buffer* ULineDrawer::XAxisVertexBuffer = nullptr;
ID3D11Buffer* ULineDrawer::YAxisVertexBuffer = nullptr;
ID3D11Buffer* ULineDrawer::ZAxisVertexBuffer = nullptr;
bool ULineDrawer::bAxisInitialized = false;

FVertexSimple XAxisVertices[] =
{
	{ 0.0f, 0.0f, 0.0f,  1.0f, 0.0f, 0.0f, 1.0f },
	{ AXIS_LENGTH, 0.0f, 0.0f,  1.0f, 0.0f, 0.0f, 1.0f }
};

// green line
FVertexSimple YAxisVertices[] =
{
	{ 0.0f, 0.0f, 0.0f,  0.0f, 0.0f, 1.0f, 1.0f },
	{ 0.0f, AXIS_LENGTH, 0.0f,  0.0f, 0.0f, 1.0f, 1.0f }
};

// blue line
FVertexSimple ZAxisVertices[] =
{
	{ 0.0f, 0.0f, 0.0f,  0.0f, 0.0f, 1.0f, 1.0f },
	{ 0.0f, 0.0f, AXIS_LENGTH,  0.0f, 0.0f, 1.0f, 1.0f }
};

ULineDrawer::ULineDrawer(ID3D11Device* Device, ID3D11DeviceContext* DeviceContext)
	: Device(Device), DeviceContext(DeviceContext), VertexBuffer(nullptr)
{
}

void ULineDrawer::Render()
{
	UINT Offset = 0;
	DeviceContext->IASetVertexBuffers(0, 1, &VertexBuffer, &Stride, &Offset);
	DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);

	DeviceContext->Draw(VertexCount, 0);
}

void ULineDrawer::Release()
{
	if (VertexBuffer)
	{
		VertexBuffer->Release();
		VertexBuffer = nullptr;
	}
}

void ULineDrawer::InitializeXYZAxis(ID3D11Device* Device)
{
	if (bAxisInitialized)
		return;

	// Create X Axis buffer
	TArray<FVertex> XAxisArray;
	for (int i = 0; i < sizeof(XAxisVertices) / sizeof(FVertexSimple); i++)
		XAxisArray.push_back((FVertex)XAxisVertices[i]);

	D3D11_BUFFER_DESC VertexBufferDesc = {};
	VertexBufferDesc.ByteWidth = static_cast<UINT>(XAxisArray.size() * sizeof(FVertex));
	VertexBufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
	VertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	VertexBufferDesc.CPUAccessFlags = 0;
	VertexBufferDesc.MiscFlags = 0;
	VertexBufferDesc.StructureByteStride = 0;

	D3D11_SUBRESOURCE_DATA VertexBufferSRD = { XAxisArray.data() };
	Device->CreateBuffer(&VertexBufferDesc, &VertexBufferSRD, &XAxisVertexBuffer);

	// Create Y Axis buffer
	TArray<FVertex> YAxisArray;
	for (int i = 0; i < sizeof(YAxisVertices) / sizeof(FVertexSimple); i++)
		YAxisArray.push_back((FVertex)YAxisVertices[i]);

	VertexBufferDesc.ByteWidth = static_cast<UINT>(YAxisArray.size() * sizeof(FVertex));
	VertexBufferSRD.pSysMem = YAxisArray.data();
	Device->CreateBuffer(&VertexBufferDesc, &VertexBufferSRD, &YAxisVertexBuffer);

	// Create Z Axis buffer
	TArray<FVertex> ZAxisArray;
	for (int i = 0; i < sizeof(ZAxisVertices) / sizeof(FVertexSimple); i++)
		ZAxisArray.push_back((FVertex)ZAxisVertices[i]);

	VertexBufferDesc.ByteWidth = static_cast<UINT>(ZAxisArray.size() * sizeof(FVertex));
	VertexBufferSRD.pSysMem = ZAxisArray.data();
	Device->CreateBuffer(&VertexBufferDesc, &VertexBufferSRD, &ZAxisVertexBuffer);

	bAxisInitialized = true;
}

void ULineDrawer::RenderXYZAxis(ID3D11DeviceContext* DeviceContext)
{
	if (!bAxisInitialized)
		return;

	// Get Default shaders from AssetManager
	auto& AssetManager = UAssetManager::GetInstance();
	auto VertexShader = AssetManager.GetVertexShader("DefaultVertexShader");
	auto PixelShader = AssetManager.GetPixelShader("DefaultPixelShader");

	if (!VertexShader || !PixelShader)
		return;

	// Set shaders
	VertexShader->Bind(DeviceContext);
	PixelShader->Bind(DeviceContext);

	UINT Stride = sizeof(FVertex);
	UINT Offset = 0;

	//// Render X Axis
	//DeviceContext->IASetVertexBuffers(0, 1, &XAxisVertexBuffer, &Stride, &Offset);
	//DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
	//DeviceContext->Draw(2, 0);

	// Render Y Axis
	DeviceContext->IASetVertexBuffers(0, 1, &YAxisVertexBuffer, &Stride, &Offset);
	DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
	DeviceContext->Draw(2, 0);

	//// Render Z Axis
	//DeviceContext->IASetVertexBuffers(0, 1, &ZAxisVertexBuffer, &Stride, &Offset);
	//DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
	//DeviceContext->Draw(2, 0);
}

void ULineDrawer::ReleaseXYZAxis()
{
	if (XAxisVertexBuffer)
	{
		XAxisVertexBuffer->Release();
		XAxisVertexBuffer = nullptr;
	}
	if (YAxisVertexBuffer)
	{
		YAxisVertexBuffer->Release();
		YAxisVertexBuffer = nullptr;
	}
	if (ZAxisVertexBuffer)
	{
		ZAxisVertexBuffer->Release();
		ZAxisVertexBuffer = nullptr;
	}
	bAxisInitialized = false;
}

