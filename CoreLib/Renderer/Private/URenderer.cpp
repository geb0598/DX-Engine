#include "Renderer/Public/URenderer.h"
#include <stdexcept>


void URenderer::Create(HWND hWindow)
{
	// Direct3D ��ġ �� ���� ü�� ����
	CreateDeviceAndSwapChain(hWindow);

	// ������ ���� ����
	CreateFrameBuffer();

	// �����Ͷ����� ���� ����
	CreateRasterizerState();

	// ���� ���ٽ� ���� �� ������ ���´� �� �ڵ忡���� �ٷ��� ����
}

void URenderer::CreateShader()
{
	ID3DBlob* vertexshaderCSO;
	ID3DBlob* pixelshaderCSO;

	D3DCompileFromFile(
		L"Shader/ShaderW0.hlsl",
		nullptr,
		nullptr,
		"mainVS",
		"vs_5_0",
		0,
		0,
		&vertexshaderCSO,
		0
	);

	Device->CreateVertexShader(
		vertexshaderCSO->GetBufferPointer(),
		vertexshaderCSO->GetBufferSize(),
		nullptr,
		&SimpleVertexShader
	);

	D3DCompileFromFile(
		L"Shader/ShaderW0.hlsl",
		nullptr,
		nullptr,
		"mainPS",
		"ps_5_0",
		0,
		0,
		&pixelshaderCSO,
		0
	);

	Device->CreatePixelShader(
		pixelshaderCSO->GetBufferPointer(),
		pixelshaderCSO->GetBufferSize(),
		nullptr,
		&SimplePixelShader
	);

	D3D11_INPUT_ELEMENT_DESC layout[] =
	{
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
	};

	Device->CreateInputLayout(
		layout,
		ARRAYSIZE(layout),
		vertexshaderCSO->GetBufferPointer(),
		vertexshaderCSO->GetBufferSize(),
		&SimpleInputLayout
	);

	Stride = sizeof(FVertexSimple);

	vertexshaderCSO->Release();
	pixelshaderCSO->Release();
}

void URenderer::ReleaseShader()
{
	if (SimpleInputLayout)
	{
		SimpleInputLayout->Release();
		SimpleInputLayout = nullptr;
	}

	if (SimplePixelShader)
	{
		SimplePixelShader->Release();
		SimplePixelShader = nullptr;
	}

	if (SimpleVertexShader)
	{
		SimpleVertexShader->Release();
		SimpleVertexShader = nullptr;
	}
}

void URenderer::CreateDeviceAndSwapChain(HWND hWindow)
{
	// �����ϴ� Direct3D ��� ������ ����
	D3D_FEATURE_LEVEL featurelevels[] = { D3D_FEATURE_LEVEL_11_0 };

	RECT rc;
	GetClientRect(hWindow, &rc);

	// ���� ü�� ���� ����ü �ʱ�ȭ
	DXGI_SWAP_CHAIN_DESC swapchaindesc = {};
	swapchaindesc.BufferDesc.Width = static_cast<UINT>(rc.right - rc.left);
	swapchaindesc.BufferDesc.Height = static_cast<UINT>(rc.bottom - rc.top);
	swapchaindesc.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;	// ���� ����
	swapchaindesc.SampleDesc.Count = 1;								//
	
	// ��Ƽ ���ø� ��Ȱ��ȭ
	swapchaindesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;	// ���� Ÿ������ ���
	swapchaindesc.BufferCount = 2;									// ���� ���۸�
	swapchaindesc.OutputWindow = hWindow;							// �������� â �ڵ�
	swapchaindesc.Windowed = TRUE;									// â ���
	swapchaindesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;		// ���� ���

	// Direct3D ��ġ�� ���� ü���� ����
	D3D11CreateDeviceAndSwapChain(
								nullptr,
								D3D_DRIVER_TYPE_HARDWARE,
								nullptr,
								D3D11_CREATE_DEVICE_BGRA_SUPPORT | D3D11_CREATE_DEVICE_DEBUG,
								featurelevels,
								ARRAYSIZE(featurelevels),
								D3D11_SDK_VERSION,
								&swapchaindesc,
								&SwapChain,
								&Device,
								nullptr,
								&DeviceContext
								);

	SwapChain->GetDesc(&swapchaindesc);

	ViewportInfo = {
					0.0f,
					0.0f,
					(float)swapchaindesc.BufferDesc.Width,
					(float)swapchaindesc.BufferDesc.Height,
					0.0f,
					1.0f
					};
}

void URenderer::ReleaseDeviceAndSwapChain()
{
	if (DeviceContext)
	{
		DeviceContext->Flush();	// �����ִ� GPU ���� ����
	}
	
	if (SwapChain)
	{
		SwapChain->Release();
		SwapChain = nullptr;
	}

	if (Device)
	{
		Device->Release();
		Device = nullptr;
	}

	if (DeviceContext)
	{
		DeviceContext->Release();
		DeviceContext = nullptr;
	}
}

void URenderer::CreateFrameBuffer()
{
	// ���� ü�����κ��� �� �۹� �ؽ�ó ��������
	SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&FrameBuffer);

	// ���� Ÿ�� �� ����
	D3D11_RENDER_TARGET_VIEW_DESC framebufferRTVdesc = {};
	framebufferRTVdesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;		// ���� ����
	framebufferRTVdesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;	// 2D �ؽ�ó

	Device->CreateRenderTargetView(
		FrameBuffer,
		&framebufferRTVdesc,
		&FrameBufferRTV
	);
}

void URenderer::ReleaseFrameBuffer()
{
	if (FrameBuffer)
	{
		FrameBuffer->Release();
		FrameBuffer = nullptr;
	}

	if (FrameBufferRTV)
	{
		FrameBufferRTV->Release();
		FrameBufferRTV = nullptr;
	}
}

ID3D11Buffer* URenderer::CreateVertexBuffer(FVertexSimple* vertices, UINT byteWidth)
{
	// 2. Create a vertex buffer
	D3D11_BUFFER_DESC vertexbufferdesc = {};
	vertexbufferdesc.ByteWidth = byteWidth;
	vertexbufferdesc.Usage = D3D11_USAGE_IMMUTABLE; // will never be updated 
	vertexbufferdesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

	D3D11_SUBRESOURCE_DATA vertexbufferSRD = { vertices };

	ID3D11Buffer* vertexBuffer;

	Device->CreateBuffer(&vertexbufferdesc, &vertexbufferSRD, &vertexBuffer);

	return vertexBuffer;
}

void URenderer::ReleaseVertexBuffer(ID3D11Buffer* vertexBuffer)
{
	if (vertexBuffer)
		vertexBuffer->Release();
}

void URenderer::CreateRasterizerState()
{
	D3D11_RASTERIZER_DESC rasterizerdesc = {};

	rasterizerdesc.FillMode = D3D11_FILL_SOLID;	// ä��� ���
	rasterizerdesc.CullMode = D3D11_CULL_BACK;	// �� ���̽� �ø�

	Device->CreateRasterizerState(&rasterizerdesc, &RasterizerState);
}

void URenderer::ReleaseRasterizerState()
{
	if (RasterizerState)
	{
		RasterizerState->Release();
		RasterizerState = nullptr;
	}
}

void URenderer::CreateConstantBuffer()
{
	D3D11_BUFFER_DESC constantbufferdesc = {};
	constantbufferdesc.ByteWidth = sizeof(FConstants) + 0xf & 0xfffffff0;	// ensure constant buffer size if multiple of 16 bytes
	constantbufferdesc.Usage = D3D11_USAGE_DYNAMIC;							// will be updated from CPU every frame
	constantbufferdesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	constantbufferdesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

	Device->CreateBuffer(&constantbufferdesc, nullptr, &ConstantBuffer);
}

void URenderer::ReleaseConstantBuffer()
{
	if (ConstantBuffer)
	{
		ConstantBuffer->Release();
		ConstantBuffer = nullptr;
	}
}

/*
void URenderer::UpdateConstant(FVector Offset)
{
	if (ConstantBuffer)
	{
		D3D11_MAPPED_SUBRESOURCE constantbufferMSR;
		
		DeviceContext->Map(ConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &constantbufferMSR);	// update constant buffer every frame
		FConstants* constants = (FConstants*)constantbufferMSR.pData;
		{
			constants->Offset = Offset;
		}
		DeviceContext->Unmap(ConstantBuffer, 0);
	}
}
*/

void URenderer::Release()
{
	RasterizerState->Release();

	// ���� Ÿ���� �ʱ�ȭ
	DeviceContext->OMSetRenderTargets(0, nullptr, nullptr);

	ReleaseFrameBuffer();
	ReleaseDeviceAndSwapChain();
}

void URenderer::SwapBuffer()
{
	SwapChain->Present(1, 0);	// 1: VSync Ȱ��ȭ
}

void URenderer::Prepare()
{
	DeviceContext->ClearRenderTargetView(FrameBufferRTV, ClearColor);

	DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	DeviceContext->RSSetViewports(1, &ViewportInfo);
	DeviceContext->RSSetState(RasterizerState);

	DeviceContext->OMSetRenderTargets(1, &FrameBufferRTV, nullptr);
	DeviceContext->OMSetBlendState(nullptr, nullptr, 0xffffffff);
}

void URenderer::PrepareShader()
{
	DeviceContext->VSSetShader(SimpleVertexShader, nullptr, 0);
	DeviceContext->PSSetShader(SimplePixelShader, nullptr, 0);
	DeviceContext->IASetInputLayout(SimpleInputLayout);

	// ���ؽ� ���̴��� ��� ���۸� �����Ѵ�.
	if (ConstantBuffer)
	{
		DeviceContext->VSSetConstantBuffers(0, 1, &ConstantBuffer);
	}
}


void URenderer::RenderPrimitive(ID3D11Buffer* pBuffer, UINT numVertices)
{
	UINT offset = 0;
	DeviceContext->IASetVertexBuffers(0, 1, &pBuffer, &Stride, &offset);

	DeviceContext->Draw(numVertices, 0);
}