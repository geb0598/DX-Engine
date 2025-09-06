#pragma once

#include <d3d11.h>

#include <wrl.h>

#include "Containers/Containers.h"
#include "Types/Types.h"

class UShaderReflector
{
public:
	struct FConstantBufferInfo
	{
		UINT Size;
		UINT BindPoint;
		Microsoft::WRL::ComPtr<ID3D11Buffer> ConstantBuffer;
	};

public:
	~UShaderReflector() = default;

	UShaderReflector(ID3D11Device* Device, ID3DBlob* ShaderBlob);

	UShaderReflector(const UShaderReflector&) = delete;
	UShaderReflector(UShaderReflector&&) noexcept = default;

	UShaderReflector& operator=(const UShaderReflector&) = delete;
	UShaderReflector& operator=(UShaderReflector&&) noexcept = default;

	FConstantBufferInfo GetConstantBufferInfo(const FString& BufferName);

	void Bind();

private:
	TMap<FString, FConstantBufferInfo> ConstantBufferInfoMap;
};
