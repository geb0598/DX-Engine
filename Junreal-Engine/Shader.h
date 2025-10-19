#pragma once
#include "ResourceBase.h"

class UShader : public UResourceBase
{
public:
	DECLARE_CLASS(UShader, UResourceBase)

	/**
	 * @brief 커맨드라인 형식의 문자열을 파싱하여 셰이더를 로드하고 컴파일합니다.
	 * @param InCommandString 셰이더 파일 경로, 매크로, 진입점 등을 포함하는 명령어 문자열.
	 * 포맷: "filepath [-D MACRO=VALUE] [-VS vs_entry] [-PS ps_entry]"
	 * - **filepath**: 컴파일할 .hlsl 파일의 경로.
	 * - **-D [MACRO_NAME](=[VALUE])**: 전처리기 매크로를 정의합니다. VALUE가 없으면 1로 정의됩니다. (예: -D USE_TEXTURE=1)
	 * - **-VS [entry_point]**: Vertex Shader의 진입점 함수 이름을 지정합니다. (기본값: mainVS)
	 * - **-PS [entry_point]**: Pixel Shader의 진입점 함수 이름을 지정합니다. (기본값: mainPS)
	 * @param InDevice 셰이더 객체를 생성할 D3D11 디바이스.
	 */
	void Load(const FString& InCommandString, ID3D11Device* InDevice);

	ID3D11InputLayout* GetInputLayout() const { return InputLayout; }
	ID3D11VertexShader* GetVertexShader() const { return VertexShader; }
	ID3D11PixelShader* GetPixelShader() const { return PixelShader; }
protected:
	virtual ~UShader();

private:
	ID3DBlob* VSBlob = nullptr;
	ID3DBlob* PSBlob = nullptr;

	ID3D11InputLayout* InputLayout = nullptr;
	ID3D11VertexShader* VertexShader = nullptr;
	ID3D11PixelShader* PixelShader = nullptr;

	void CreateInputLayout(ID3D11Device* Device, const FString& InShaderPath);
	void ReleaseResources();
};

struct FVertexPositionColor
{
	static const D3D11_INPUT_ELEMENT_DESC* GetLayout()
	{
		static const D3D11_INPUT_ELEMENT_DESC layout[] = {
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 }
		};
		return layout;
	}
	static uint32 GetLayoutCount() { return 2; }
};

struct FVertexPositionColorTexturNormal
{
	static const D3D11_INPUT_ELEMENT_DESC* GetLayout()
	{
		static const D3D11_INPUT_ELEMENT_DESC layout[] = {
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 40, D3D11_INPUT_PER_VERTEX_DATA, 0 }
		};
		return layout;
	}

	static uint32 GetLayoutCount() { return 4; }
};

struct FVertexPositionBillBoard

{

	static const D3D11_INPUT_ELEMENT_DESC* GetLayout()

	{

		static const D3D11_INPUT_ELEMENT_DESC layout[] = {

		{ "WORLDPOSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },

		{ "SIZE", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},

		{ "UVRECT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 20, D3D11_INPUT_PER_VERTEX_DATA, 0},

		};

		return layout;

	}



	static uint32 GetLayoutCount() { return 3; }

};