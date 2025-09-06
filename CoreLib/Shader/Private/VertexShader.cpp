#include <d3d11.h>
#include <d3dcompiler.h>

#include "Types/Types.h"     
#include "Shader/Public/VertexShader.h"

UVertexShader::UVertexShader(
	ID3D11Device* Device, 
	const TArray<D3D11_INPUT_ELEMENT_DESC>& InputElementDesc,
	const std::filesystem::path& VertexShaderFilePath,
	const FString& VertexShaderMain
)
{
	if (!std::filesystem::exists(VertexShaderFilePath))
	{
		throw std::invalid_argument("File not exist: " + VertexShaderFilePath.string());
	}

	Microsoft::WRL::ComPtr<ID3DBlob> ShaderBlob;
	Microsoft::WRL::ComPtr<ID3DBlob> ShaderErrorBlob;

	HRESULT hResult = D3DCompileFromFile(
		VertexShaderFilePath.wstring().c_str(),
		nullptr,
		nullptr,
		VertexShaderMain.c_str(),
		"vs_5_0",
		0,
		0,
		ShaderBlob.ReleaseAndGetAddressOf(),
		ShaderErrorBlob.ReleaseAndGetAddressOf()
	);

	// TODO: Improve Error Handling
	if (FAILED(hResult))
	{
		if (ShaderErrorBlob)
		{
			throw std::runtime_error(
				"Vertex Shader Compilation Failed: " +
				std::string(static_cast<const char*>(ShaderErrorBlob->GetBufferPointer()))
			);
		}
		else
		{
			throw std::runtime_error("Vertex Shader Compilation Failed");
		}
	}

	Device->CreateVertexShader(
		ShaderBlob->GetBufferPointer(),
		ShaderBlob->GetBufferSize(),
		nullptr,
		VertexShader.ReleaseAndGetAddressOf()
	);

	ShaderReflector = std::make_unique<UShaderReflector>(Device, ShaderBlob.Get());

	Device->CreateInputLayout(
		InputElementDesc.data(),
		InputElementDesc.size(),
		ShaderBlob->GetBufferPointer(),
		ShaderBlob->GetBufferSize(),
		InputLayout.ReleaseAndGetAddressOf()
	);
}

/*
void UVertexShader::Bind(ID3D11DeviceContext* DeviceContext) const
{
	DeviceContext->IASetInputLayout(InputLayout.Get());

	DeviceContext->VSSetShader(VertexShader.Get(), nullptr, 0);

	// ShaderReflector->Bind(BufferName);
}

*/
