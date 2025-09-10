#include "../Public/UGridManager.h"

UGridManager::UGridManager(ID3D11Device* Device, ID3D11DeviceContext* DeviceContext)
	: Device(Device),
	DeviceContext(DeviceContext),
	VertexShader(nullptr),
	PixelShader(nullptr),
	InputLayout(nullptr),
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

	// compile vertex shaders
	ID3DBlob* VertexShaderBlob = nullptr;
	ID3DBlob* VertexShaderErrorBlob = nullptr;

	HResult = D3DCompileFromFile(
		L"./Shader/GridVertexShader.hlsl",
		nullptr,
		nullptr,
		"GridVS",
		"vs_5_0",
		0,
		0,
		&VertexShaderBlob,
		&VertexShaderErrorBlob
	);

	// TODO: Improve Error Handling
	if (FAILED(HResult))
	{
		exit(1);
	}

	HResult = Device->CreateVertexShader(
		VertexShaderBlob->GetBufferPointer(),
		VertexShaderBlob->GetBufferSize(),
		nullptr,
		&VertexShader
	);

	// TODO: Improve Error Handling
	if (FAILED(HResult))
	{
		exit(1);
	}

	D3D11_INPUT_ELEMENT_DESC InputLayoutDesc[] =
	{
		{"POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
	};

	HResult = Device->CreateInputLayout(
		InputLayoutDesc,
		ARRAYSIZE(InputLayoutDesc),
		VertexShaderBlob->GetBufferPointer(),
		VertexShaderBlob->GetBufferSize(),
		&InputLayout
	);

	if (FAILED(HResult))
		exit(1);

	if (VertexShaderBlob)
		VertexShaderBlob->Release();

	if (VertexShaderErrorBlob)
		VertexShaderErrorBlob->Release();

	// create pixelshader

	ID3DBlob* PixelShaderBlob = nullptr;
	ID3DBlob* PixelShaderErrorBlob = nullptr;

	HResult = D3DCompileFromFile(
		L"./Shader/GridPixelShader.hlsl",
		nullptr,
		nullptr,
		"GridPS",
		"ps_5_0",
		0,
		0,
		&PixelShaderBlob,
		&PixelShaderErrorBlob
	);

	// TODO: Improve Error Handling
	if (FAILED(HResult))
	{
		exit(1);
	}

	HResult = Device->CreatePixelShader(
		PixelShaderBlob->GetBufferPointer(),
		PixelShaderBlob->GetBufferSize(),
		nullptr,
		&PixelShader
	);

	// TODO: Improve Error Handling
	if (FAILED(HResult))
	{
		exit(1);
	}

	if (PixelShaderBlob)
		PixelShaderBlob->Release();
	if (PixelShaderErrorBlob)
		PixelShaderErrorBlob->Release();

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
	cbDesc.Usage = D3D11_USAGE_DEFAULT;        // GPU에서 업데이트 가능
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
	DeviceContext->IASetInputLayout(InputLayout);
	DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	UINT Offset = 0;
	DeviceContext->IASetVertexBuffers(0, 1, &VertexBuffer, &Stride, &Offset);

	DeviceContext->VSSetShader(VertexShader, nullptr, 0);

	DeviceContext->PSSetShader(PixelShader, nullptr, 0);
	DeviceContext->UpdateSubresource(PSConstantBuffer, 0, nullptr, &GridInfo, 0, 0);
	DeviceContext->PSSetConstantBuffers(
		1,              // slot num (HLSL에서 register(bn))
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
	if (VertexShader)
	{
		VertexShader->Release();
		VertexShader = nullptr;
	}
	if (PixelShader)
	{
		PixelShader->Release();
		PixelShader = nullptr;
	}
	if (InputLayout)
	{
		InputLayout->Release();
		InputLayout = nullptr;
	}
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