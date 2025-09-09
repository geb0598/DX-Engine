#pragma once

// D3D 사용에 필요한 헤더파일들을 포함한다.
#include <d3d11.h>
#include <d3dcompiler.h>

#include <wrl.h>

// Direct3D 11 사용에 필요한 기본 생성, 소멸 기능을 담당한다.
class URenderer
{
public:
	static URenderer & GetInstance();

	~URenderer();

	URenderer(const URenderer&) = delete;
	URenderer(URenderer&&) = delete;

	URenderer& operator=(const URenderer&) = delete;
	URenderer& operator=(URenderer&&) = delete;

	void Create(HWND hWindow);						// 렌더러 초기화 함수
	void Prepare();									// 그래픽 파이프라인에 필요한 정보를 전달하고 바인드
	void SwapBuffer();								// 스왑 체인의 백 버퍼와 프론트 버퍼를 교체하여 화면에 출력

	ID3D11Device* GetDevice();
	ID3D11DeviceContext* GetDeviceContext();

private:
	URenderer();

	void CreateDeviceAndSwapChain(HWND hWindow);	// Direct3D 장치 및 스왑 체인을 생성하는 함수
	void CreateFrameBuffer();						// 프레임 버퍼를 생성하는 함수
	void CreateRasterizerState();					// 래스터라이저 상태를 생성하는 함수

	// Direct3D 11 장치(Device)와 장치 컨텍스트(Device Context) 및 스왑 체인(Swap Chain)을 관리하기 위한 포인터들
	Microsoft::WRL::ComPtr<ID3D11Device> Device;					// GPU와 통신하기 위한 Direct3D 장치
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> DeviceContext;		// GPU 명령 실행을 담당하는 컨텍스트
	Microsoft::WRL::ComPtr<IDXGISwapChain> SwapChain; 				// 프레임 버퍼를 교체하는 데 사용되는 스왑 체인

	// 렌더링에 필요한 리소스 및 상태를 관리하기 위한 변수들
	Microsoft::WRL::ComPtr<ID3D11Texture2D> FrameBuffer;			// 화면 출력용 텍스처
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> FrameBufferRTV;	// 텍스처를 렌더 타켓으로 사용하는 뷰
	Microsoft::WRL::ComPtr<ID3D11RasterizerState> RasterizerState;	// 래스터라이저 상태(컬링, 채우기 모드 등 정의)

	Microsoft::WRL::ComPtr<ID3D11Texture2D> DepthStencilBuffer;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView> DepthStencilView;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilState> DepthStencilState;

	FLOAT ClearColor[4] = {0.025f, 0.025f, 0.025f, 1.0f};			// 화면을 초기화(clear)할 때 사용할 색상 (RGBA)
	D3D11_VIEWPORT ViewportInfo;									// 렌더링 영역을 정의하는 뷰포트 정보
};
