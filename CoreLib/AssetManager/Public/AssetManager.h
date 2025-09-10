#pragma once

#include <memory>
#include <filesystem>

#include <d3d11.h>

#include "Containers/Containers.h"
#include "Types/Types.h"

// Forward declarations
class UMesh;
class UVertexShader;
class UPixelShader;
struct FVertex;

class UAssetManager
{
public:
	static UAssetManager& GetInstance();

	~UAssetManager() = default;

	UAssetManager(const UAssetManager&) = delete;
	UAssetManager(UAssetManager&&) = delete;

	UAssetManager& operator=(const UAssetManager&) = delete;
	UAssetManager& operator=(UAssetManager&&) = delete;

	void Initialize(ID3D11Device* Device);

	std::shared_ptr<UMesh> GetOrCreateMesh(const FString& MeshName, const TArray<FVertex>& VertexArray);
	std::shared_ptr<UVertexShader> GetOrCreateVertexShader(
		const FString& ShaderName,
		const std::filesystem::path& VertexShaderFilePath,
		const FString& VertexShaderMain,
		const TArray<D3D11_INPUT_ELEMENT_DESC>& InputLayoutDesc
	);
	std::shared_ptr<UPixelShader> GetOrCreatePixelShader(
		const FString& ShaderName,
		const std::filesystem::path& PixelShaderFilePath,
		const FString& PixelShaderMain
	);

	std::shared_ptr<UMesh> GetMesh(const FString& MeshName);
	std::shared_ptr<UVertexShader> GetVertexShader(const FString& ShaderName);
	std::shared_ptr<UPixelShader> GetPixelShader(const FString& ShaderName);

	void ClearAllResources();

private:
	UAssetManager() = default;

	ID3D11Device* Device = nullptr;

	TMap<FString, std::shared_ptr<UMesh>> Meshes;
	TMap<FString, std::shared_ptr<UVertexShader>> VertexShaders;
	TMap<FString, std::shared_ptr<UPixelShader>> PixelShaders;
};