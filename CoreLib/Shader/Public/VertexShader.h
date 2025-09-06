#pragma once

#include <filesystem>
#include <memory>

#include <d3d11.h>

#include "Types/Types.h"
#include "Shader/Private/ShaderReflector.h"

class UVertexShader
{
public:
	~UVertexShader() = default;

	UVertexShader(
		ID3D11Device* Device,
		const TArray<D3D11_INPUT_ELEMENT_DESC>& InputElementDesc,
		const std::filesystem::path& VertexShaderFilePath,
		const FString& VertexShaderMain
	);

	UVertexShader(const UVertexShader&) = delete;
	UVertexShader(UVertexShader&&) noexcept = default;

	UVertexShader& operator=(const UVertexShader&) = delete;
	UVertexShader& operator=(UVertexShader&&) noexcept = default;

	template<typename TBuffer>
	void UpdateConstantBuffer(ID3D11DeviceContext* DeviceContext, const FString& BufferName, const void* BufferData) const;

	template<typename... TBufferNames>
	std::enable_if_t<(std::is_same_v<TBufferNames, FString> && ...), void>
		Bind(ID3D11DeviceContext* DeviceContext, TBufferNames&&... BufferNames) const;

private:
	std::unique_ptr<UShaderReflector> ShaderReflector;

	Microsoft::WRL::ComPtr<ID3D11VertexShader> VertexShader;
	Microsoft::WRL::ComPtr<ID3D11InputLayout> InputLayout;
};

template<typename TBuffer>
inline void UVertexShader::UpdateConstantBuffer(ID3D11DeviceContext* DeviceContext, const FString& BufferName, const void* Buff) const
{
	auto ConstantBufferInfo = ShaderReflector->GetConstantBufferInfo(BufferName);
	DeviceContext->UpdateSubresource(
		ConstantBufferInfo.ConstantBuffer.Get(),
		0, 
		nullptr, 
		BufferData, 
		0,
		0
	);
}

template<typename ...TBufferNames>
inline std::enable_if_t<(std::is_same_v<TBufferNames, FString> && ...), void>
	UVertexShader::Bind(ID3D11DeviceContext* DeviceContext, TBufferNames && ...BufferNames) const
{
	DeviceContext->IASetInputLayout(InputLayout.Get());

	DeviceContext->VSSetShader(VertexShader.Get(), nullptr, 0);

	(ShaderReflector->Bind(DeviceContext, std::forward<TBufferNames>(BufferNames)), ...);
}
