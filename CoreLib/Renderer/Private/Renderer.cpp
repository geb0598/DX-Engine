#include "Renderer/Public/Renderer.h"

// NOTE: Renderer is initialized once per execution as a singleton
URenderer & URenderer::GetInstance()
{
	static URenderer Renderer;
	return Renderer;
}

URenderer::~URenderer()
{
	if (DeviceContext)
	{
		DeviceContext->Flush();
	}
}

void URenderer::Prepare()
{
	DeviceContext->ClearRenderTargetView(FrameBufferRTV.Get(), ClearColor);
	DeviceContext->ClearDepthStencilView(DepthStencilView.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);

	DeviceContext->RSSetViewports(1, &ViewportInfo);
	DeviceContext->RSSetState(RasterizerState.Get());

	DeviceContext->OMSetRenderTargets(1, FrameBufferRTV.GetAddressOf(), DepthStencilView.Get());
	DeviceContext->OMSetBlendState(nullptr, nullptr, 0xffffffff);
	DeviceContext->OMSetDepthStencilState(DepthStencilState.Get(), 1);
}

void URenderer::SwapBuffer()
{
	SwapChain->Present(1, 0);	// 1: VSync 활성화
}

ID3D11Device* URenderer::GetDevice()
{
	return Device.Get();
}

ID3D11DeviceContext* URenderer::GetDeviceContext()
{
	return DeviceContext.Get();
}

void URenderer::ResizeBuffers(int Width, int Height)
{
	// 렌더 타겟 뷰를 해제
	FrameBufferRTV.Reset();
	FrameBuffer.Reset();
	
	// 깊이 스텐실 뷰와 버퍼도 해제
	DepthStencilView.Reset();
	DepthStencilBuffer.Reset();
	
	// 스왑 체인 버퍼 크기 조정
	HRESULT hr = SwapChain->ResizeBuffers(0, Width, Height, DXGI_FORMAT_UNKNOWN, 0);
	if (FAILED(hr))
	{
		// 에러 처리
		return;
	}
	
	// 새로운 크기로 프레임 버퍼 다시 생성
	CreateFrameBuffer();

	// 새로운 크기로 깊이 스텐실 버퍼 다시 생성
	CreateDepthStencilBuffer(Width, Height);

	// 뷰포트 크기 업데이트
	ViewportInfo.Width = (float)Width;
	ViewportInfo.Height = (float)Height;
}

URenderer::URenderer()
{

}

void URenderer::Create(HWND hWnd)
{
	// Direct3D 장치 및 스왑 체인 생성
	CreateDeviceAndSwapChain(hWnd);

	// 프레임 버퍼 생성
	CreateFrameBuffer();

	// 래스터라이저 상태 생성
	CreateRasterizerState();
}

void URenderer::CreateDeviceAndSwapChain(HWND hWnd)
{
	D3D_FEATURE_LEVEL FeatureLevels[] = { D3D_FEATURE_LEVEL_11_0 };
	DXGI_SWAP_CHAIN_DESC SwapChainDesc = {};
	SwapChainDesc.BufferDesc.Width = 0;
	SwapChainDesc.BufferDesc.Height = 0;
	SwapChainDesc.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	SwapChainDesc.SampleDesc.Count = 1;
	SwapChainDesc.SampleDesc.Quality = 0;
	SwapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	SwapChainDesc.BufferCount = 2;
	SwapChainDesc.OutputWindow = hWnd;
	SwapChainDesc.Windowed = TRUE;
	SwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

	D3D11CreateDeviceAndSwapChain(
		nullptr, 
		D3D_DRIVER_TYPE_HARDWARE, 
		nullptr,
		D3D11_CREATE_DEVICE_BGRA_SUPPORT | D3D11_CREATE_DEVICE_DEBUG,
		FeatureLevels, 
		ARRAYSIZE(FeatureLevels), 
		D3D11_SDK_VERSION,
		&SwapChainDesc, 
		&SwapChain, 
		&Device, 
		nullptr, 
		&DeviceContext
	);

	// 생성된 스왑 체인의 정보 가져오기
	SwapChain->GetDesc(&SwapChainDesc);

	// 뷰포트 정보 설정
	ViewportInfo.TopLeftX = 0.0f;
	ViewportInfo.TopLeftY = 0.0f;
	ViewportInfo.Width = (float)SwapChainDesc.BufferDesc.Width;
	ViewportInfo.Height = (float)SwapChainDesc.BufferDesc.Height;
	ViewportInfo.MinDepth = 0.0f;
	ViewportInfo.MaxDepth = 1.0f;

	// 깊이 스텐실 버퍼 생성
	CreateDepthStencilBuffer(SwapChainDesc.BufferDesc.Width, SwapChainDesc.BufferDesc.Height);

	D3D11_DEPTH_STENCIL_DESC DepthStencilStateDesc = {};
	DepthStencilStateDesc.DepthEnable = TRUE;
	DepthStencilStateDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	DepthStencilStateDesc.DepthFunc = D3D11_COMPARISON_LESS;
	DepthStencilStateDesc.StencilEnable = FALSE;

	Device->CreateDepthStencilState(&DepthStencilStateDesc, DepthStencilState.ReleaseAndGetAddressOf());
}

void URenderer::CreateFrameBuffer()
{
	// 스왑 체인으로부터 백 퍼버 텍스처 가져오기
	SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)FrameBuffer.ReleaseAndGetAddressOf());

	// 렌더 타겟 뷰 생성
	D3D11_RENDER_TARGET_VIEW_DESC FramebufferRTVdesc = {};
	FramebufferRTVdesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;		// 색상 포맷
	FramebufferRTVdesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;	// 2D 텍스처

	Device->CreateRenderTargetView(
		FrameBuffer.Get(),
		&FramebufferRTVdesc,
		FrameBufferRTV.ReleaseAndGetAddressOf()
	);
}

void URenderer::CreateDepthStencilBuffer(int Width, int Height)
{
	D3D11_TEXTURE2D_DESC DepthStencilDesc = {};
	DepthStencilDesc.Width = Width;
	DepthStencilDesc.Height = Height;
	DepthStencilDesc.MipLevels = 1;
	DepthStencilDesc.ArraySize = 1;
	DepthStencilDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	DepthStencilDesc.SampleDesc.Count = 1;
	DepthStencilDesc.SampleDesc.Quality = 0;
	DepthStencilDesc.Usage = D3D11_USAGE_DEFAULT;
	DepthStencilDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	DepthStencilDesc.CPUAccessFlags = 0;
	DepthStencilDesc.MiscFlags = 0;

	Device->CreateTexture2D(&DepthStencilDesc, nullptr, DepthStencilBuffer.ReleaseAndGetAddressOf());

	Device->CreateDepthStencilView(DepthStencilBuffer.Get(), nullptr, DepthStencilView.ReleaseAndGetAddressOf());
}

void URenderer::CreateRasterizerState()
{
	D3D11_RASTERIZER_DESC RasterizerDesc = {};

	RasterizerDesc.FillMode = D3D11_FILL_SOLID;	// 채우기 모드
	RasterizerDesc.CullMode = D3D11_CULL_BACK;	// 백 페이스 컬링
	RasterizerDesc.FrontCounterClockwise = true;

	Device->CreateRasterizerState(&RasterizerDesc, RasterizerState.ReleaseAndGetAddressOf());
}


