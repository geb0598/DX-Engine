#include "../Public/UGridManager.h"
#include "AssetManager/Public/AssetManager.h"
#include "Shader/Shader.h"
#include "Mesh/Public/Vertex.h"

UGridManager::UGridManager(ID3D11Device* Device, ID3D11DeviceContext* DeviceContext)
	: Device(Device),
	DeviceContext(DeviceContext),
	GridVertexShader(nullptr),
	GridPixelShader(nullptr),
	VertexBuffer(nullptr),
	PSConstantBuffer(nullptr),
	GridDepthStencilState(nullptr),
	ObjectDepthStencilState(nullptr),
	VertexCount(0),
	Stride(0)
{
}

void UGridManager::Initialize()
{
	// Recieve function return
	HRESULT HResult;

	// Define Input Layout
	D3D11_INPUT_ELEMENT_DESC InputLayoutDesc[] =
	{
		{"POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
	};

	TArray<D3D11_INPUT_ELEMENT_DESC> InputLayoutDescArray(
		std::begin(InputLayoutDesc),
		std::end(InputLayoutDesc)
	);

	// Get Grid shaders from AssetManager instead of creating them
	auto& AssetManager = UAssetManager::GetInstance();
	GridVertexShader = AssetManager.GetOrCreateVertexShader(
		"GridVertexShader",
		"./Shader/GridVertexShader.hlsl",
		"GridVS",
		InputLayoutDescArray
	);

	GridPixelShader = AssetManager.GetOrCreatePixelShader(
		"GridPixelShader",
		"./Shader/GridPixelShader.hlsl",
		"GridPS"
	);

	if (!GridVertexShader || !GridPixelShader)
	{
		exit(1); // TODO: Improve Error Handling
	}

	// InputLayout is managed by AssetManager's VertexShader, no need to create separately

	float VertexArray[3][2] = { {-1.0f, -1.0f}, {3.0f, -1.0f}, {-1.0, 3.0f} };

	// create vertex buffer

	VertexCount = sizeof(VertexArray) / sizeof(float[2]);
	Stride = sizeof(float[2]);

	D3D11_BUFFER_DESC VertexBufferDesc = {};
	VertexBufferDesc.ByteWidth = static_cast<UINT>(VertexCount * Stride);
	VertexBufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
	VertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	VertexBufferDesc.CPUAccessFlags = 0;
	VertexBufferDesc.MiscFlags = 0;
	VertexBufferDesc.StructureByteStride = 0;

	D3D11_SUBRESOURCE_DATA VertexBufferSRD = { };
	VertexBufferSRD.pSysMem = VertexArray;

	HResult = Device->CreateBuffer(&VertexBufferDesc, &VertexBufferSRD, &VertexBuffer);
	if (FAILED(HResult))
		exit(1);

	// create constant buffer

	D3D11_BUFFER_DESC cbDesc = {};
	cbDesc.ByteWidth = sizeof(PSConstants);
	cbDesc.Usage = D3D11_USAGE_DEFAULT;        // GPU���� ������Ʈ ����
	cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cbDesc.CPUAccessFlags = 0;
	cbDesc.MiscFlags = 0;
	cbDesc.StructureByteStride = 0;

	HResult = Device->CreateBuffer(&cbDesc, nullptr, &PSConstantBuffer);
	if (FAILED(HResult))
	{
		exit(1);
	}

	// create depth stencil state for grid
	D3D11_DEPTH_STENCIL_DESC gridDSDesc = {};
	gridDSDesc.DepthEnable = true;                       // Remain Depth Stencil Test
	gridDSDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO; // DepthWrite Off
	gridDSDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
	gridDSDesc.StencilEnable = false;

	Device->CreateDepthStencilState(&gridDSDesc, &GridDepthStencilState);

	// create depth stencil state for restore
	gridDSDesc = {};
	gridDSDesc.DepthEnable = true;                       // Remain Depth Stencil Test
	gridDSDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL; // DepthWrite Off
	gridDSDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
	gridDSDesc.StencilEnable = false;

	Device->CreateDepthStencilState(&gridDSDesc, &ObjectDepthStencilState);

}

void UGridManager::Bind(PSConstants GridInfo)
{
	DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	UINT Offset = 0;
	DeviceContext->IASetVertexBuffers(0, 1, &VertexBuffer, &Stride, &Offset);

	// Use shaders from AssetManager (InputLayout is handled by VertexShader->Bind())
	if (GridVertexShader && GridPixelShader)
	{
		GridVertexShader->Bind(DeviceContext);
		GridPixelShader->Bind(DeviceContext);
	}

	DeviceContext->UpdateSubresource(PSConstantBuffer, 0, nullptr, &GridInfo, 0, 0);
	DeviceContext->PSSetConstantBuffers(
		1,              // slot num (HLSL���� register(bn))
		1,              // buffer num
		&PSConstantBuffer
	);

	DeviceContext->OMSetDepthStencilState(GridDepthStencilState, 0);
}

void UGridManager::Render()
{
	DeviceContext->Draw(3, 0);
	DeviceContext->OMSetDepthStencilState(ObjectDepthStencilState, 0);
}

void UGridManager::Release()
{
	// Don't release shaders as they're managed by AssetManager
	GridVertexShader = nullptr;
	GridPixelShader = nullptr;

	// InputLayout is managed by AssetManager's VertexShader, so don't release it here
	if (VertexBuffer)
	{
		VertexBuffer->Release();
		VertexBuffer = nullptr;
	}
	if (PSConstantBuffer)
	{
		PSConstantBuffer->Release();
		PSConstantBuffer = nullptr;
	}
	if (GridDepthStencilState)
	{
		GridDepthStencilState->Release();
		GridDepthStencilState = nullptr;
	}
	if (ObjectDepthStencilState)
	{
		ObjectDepthStencilState->Release();
		ObjectDepthStencilState = nullptr;
	}
}