#pragma once

#include <filesystem>
#include <memory>

#include "Types/Types.h"
#include "Shader/Private/ShaderReflector.h"

class UVertexShader
{
public:
	~UVertexShader() = default;

	UVertexShader(
		ID3D11Device* Device,
		const std::filesystem::path& FilePath,
		const FString& VertexShaderMain
	);

	UVertexShader(const UVertexShader&) = delete;
	UVertexShader(UVertexShader&&) noexcept = default;

	UVertexShader& operator=(const UVertexShader&) = delete;
	UVertexShader& operator=(UVertexShader&&) noexcept = default;

	template<typename TBuffer>
	void UpdateConstantBuffer(ID3D11DeviceContext* DeviceContext, TBuffer&& Buffer) const;

	void Bind(ID3D11DeviceContext* DeviceContext) const;

private:
	std::unique_ptr<UShaderReflector> ShaderReflector;

	Microsoft::WRL::ComPtr<ID3D11VertexShader> VertexShader;
	Microsoft::WRL::ComPtr<ID3D11InputLayout> InputLayout;
};

