#pragma once

// D3D 사용에 필요한 라이브러리들을 링크한다.
#pragma comment(lib, "user32")
#pragma comment(lib, "d3d11")
#pragma comment(lib, "d3dcompiler")

// D3D 사용에 필요한 헤더파일들을 포함한다.
#include <d3d11.h>
#include <d3dcompiler.h>

// 1. Define the triangle vertices
struct FVertexSimple
{
	float x, y, z;		// Position
	float r, g, b, a;	// Color
};

// Structure for a 3D vector
/*
struct FVector
{
	float x, y, z;
	FVector(float _x = 0, float _y = 0, float _z = 0) : x(_x), y(_y), z(_z) {};
};
*/

extern FVertexSimple triangle_vertices[];
extern FVertexSimple cube_vertices[];
extern FVertexSimple sphere_vertices[];

// Direct3D 11 사용에 필요한 기본 생성, 소멸 기능을 담당한다.
class URenderer
{
public:
	// Direct3D 11 장치(Device)와 장치 컨텍스트(Device Context) 및 스왑 체인(Swap Chain)을 관리하기 위한 포인터들
	ID3D11Device* Device = nullptr;						// GPU와 통신하기 위한 Direct3D 장치
	ID3D11DeviceContext* DeviceContext = nullptr;		// GPU 명령 실행을 담당하는 컨텍스트
	IDXGISwapChain* SwapChain = nullptr;				// 프레임 버퍼를 교체하는 데 사용되는 스왑 체인

	// 렌더링에 필요한 리소스 및 상태를 관리하기 위한 변수들
	ID3D11Texture2D* FrameBuffer = nullptr;				// 화면 출력용 텍스처
	ID3D11RenderTargetView* FrameBufferRTV = nullptr;	// 텍스처를 렌더 타켓으로 사용하는 뷰
	ID3D11RasterizerState* RasterizerState = nullptr;	// 래스터라이저 상태(컬링, 채우기 모드 등 정의)
	ID3D11Buffer* ConstantBuffer = nullptr;				// 쉐이더에 데이터를 전달하기 위한 상수 버퍼

	FLOAT ClearColor[4] = {0.025f, 0.025f, 0.025f, 1.0f};	// 화면을 초기화(clear)할 때 사용할 색상 (RGBA)
	D3D11_VIEWPORT ViewportInfo;							// 렌더링 영역을 정의하는 뷰포트 정보

	// 쉐이더 관련 변수들
	ID3D11VertexShader* SimpleVertexShader = nullptr;
	ID3D11PixelShader* SimplePixelShader = nullptr;
	ID3D11InputLayout* SimpleInputLayout = nullptr;
	unsigned int Stride;

	// 상수 버퍼 정의 구조체
	struct FConstants
	{
		// FVector Offset;
		float Pad;
	};

public:
	void Create(HWND hWindow);						// 렌더러 초기화 함수
	void CreateShader();							// 쉐이더 생성 함수
	void ReleaseShader();							// 쉐이더 해제 함수
	void CreateDeviceAndSwapChain(HWND hWindow);	// Direct3D 장치 및 스왑 체인을 생성하는 함수
	void ReleaseDeviceAndSwapChain();				// Direct3D 장치 및 스왑 체인을 해제하는 함수
	void CreateFrameBuffer();						// 프레임 버퍼를 생성하는 함수
	void ReleaseFrameBuffer();						// 프레임 버퍼를 해제하는 함수
	ID3D11Buffer* CreateVertexBuffer(FVertexSimple* vertices, UINT byteWidth);	// 정점 버퍼를 생성하는 함수
	void ReleaseVertexBuffer(ID3D11Buffer* vertexBuffer);						// 정점 버퍼를 해제하는 함수
	void CreateRasterizerState();					// 래스터라이저 상태를 생성하는 함수
	void ReleaseRasterizerState();					// 래스터라이저 상태를 해제하는 함수
	void CreateConstantBuffer();					// 상수 버퍼 생성
	void ReleaseConstantBuffer();					// 상수 버퍼 해제
	// void UpdateConstant(FVector Offset);			// 상수 버퍼를 갱신하는 함수
	void Release();									// 렌더러에 사용된 모든 리소스를 해제하는 함수
	void SwapBuffer();								// 스왑 체인의 백 버퍼와 프론트 버퍼를 교체하여 화면에 출력

	void Prepare();									// 그래픽 파이프라인에 필요한 정보를 전달하고 바인드
	void PrepareShader();							// 그래픽 파이프라인에 쉐이더와 입력 레이아웃을 바인드
	void RenderPrimitive(ID3D11Buffer* pBuffer, UINT numVertices);							// 프레임 버퍼에 목표를 렌더
};
