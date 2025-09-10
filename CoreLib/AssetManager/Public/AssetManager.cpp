#include "../Public/AssetManager.h"
#include "Mesh/Public/Mesh.h"
#include "Shader/Public/VertexShader.h"
#include "Shader/Public/PixelShader.h"

UAssetManager& UAssetManager::GetInstance()
{
	static UAssetManager Instance;
	return Instance;
}

void UAssetManager::Initialize(ID3D11Device* InDevice)
{
	Device = InDevice;

	TArray<D3D11_INPUT_ELEMENT_DESC> InputLayoutDesc(
		std::begin(FVertex::InputLayoutDesc),
		std::end(FVertex::InputLayoutDesc)
	);

	GetOrCreateVertexShader(
		"DefaultVertexShader",
		"./Shader/VertexShader.hlsl",
		"main",
		InputLayoutDesc
	);

	GetOrCreatePixelShader(
		"DefaultPixelShader",
		"./Shader/PixelShader.hlsl",
		"main"
	);
}

std::shared_ptr<UMesh> UAssetManager::GetOrCreateMesh(const FString& MeshName, const TArray<FVertex>& VertexArray)
{
	auto It = Meshes.find(MeshName);
	if (It != Meshes.end())
	{
		return It->second;
	}

	if (!Device)
	{
		return nullptr;
	}

	auto NewMesh = std::make_shared<UMesh>(Device, VertexArray);
	Meshes[MeshName] = NewMesh;
	return NewMesh;
}

std::shared_ptr<UVertexShader> UAssetManager::GetOrCreateVertexShader(
	const FString& ShaderName,
	const std::filesystem::path& VertexShaderFilePath,
	const FString& VertexShaderMain,
	const TArray<D3D11_INPUT_ELEMENT_DESC>& InputLayoutDesc)
{
	auto It = VertexShaders.find(ShaderName);
	if (It != VertexShaders.end())
	{
		return It->second;
	}

	if (!Device)
	{
		return nullptr;
	}

	auto NewVertexShader = std::make_shared<UVertexShader>(Device, VertexShaderFilePath, VertexShaderMain, InputLayoutDesc);
	VertexShaders[ShaderName] = NewVertexShader;
	return NewVertexShader;
}

std::shared_ptr<UPixelShader> UAssetManager::GetOrCreatePixelShader(
	const FString& ShaderName,
	const std::filesystem::path& PixelShaderFilePath,
	const FString& PixelShaderMain)
{
	auto It = PixelShaders.find(ShaderName);
	if (It != PixelShaders.end())
	{
		return It->second;
	}

	if (!Device)
	{
		return nullptr;
	}

	auto NewPixelShader = std::make_shared<UPixelShader>(Device, PixelShaderFilePath, PixelShaderMain);
	PixelShaders[ShaderName] = NewPixelShader;
	return NewPixelShader;
}

std::shared_ptr<UMesh> UAssetManager::GetMesh(const FString& MeshName)
{
	auto It = Meshes.find(MeshName);
	if (It != Meshes.end())
	{
		return It->second;
	}
	return nullptr;
}

std::shared_ptr<UVertexShader> UAssetManager::GetVertexShader(const FString& ShaderName)
{
	auto It = VertexShaders.find(ShaderName);
	if (It != VertexShaders.end())
	{
		return It->second;
	}
	return nullptr;
}

std::shared_ptr<UPixelShader> UAssetManager::GetPixelShader(const FString& ShaderName)
{
	auto It = PixelShaders.find(ShaderName);
	if (It != PixelShaders.end())
	{
		return It->second;
	}
	return nullptr;
}

void UAssetManager::ClearAllResources()
{
	Meshes.clear();
	VertexShaders.clear();
	PixelShaders.clear();
}