#pragma once
#include "fbxsdk.h"
#include <filesystem>
#include "Global/Vector.h"
#include "Global/Types.h"

struct FFbxMaterialInfo
{
	std::string MaterialName;
	std::filesystem::path DiffuseTexturePath;
};

struct FFbxMeshSection
{
	uint32 StartIndex;
	uint32 IndexCount;
	uint32 MaterialIndex;
};

struct FFbxMeshInfo
{
	TArray<FVector> VertexList;
	TArray<FVector> NormalList;
	TArray<FVector2> TexCoordList;
	TArray<uint32> Indices;

	TArray<FFbxMaterialInfo> Materials;
	TArray<FFbxMeshSection> Sections;
};

class FFbxImporter
{
public:
	struct Configuration
	{
		bool bConvertToUEBasis = true;
	};

	// 🔸 FBX SDK 세션 관리
	static bool Initialize();
	static void Shutdown();

	static bool LoadFBX(
		const std::filesystem::path& FilePath,
		FFbxMeshInfo* OutMeshInfo,
		Configuration Config = {});

private:
	// Helper functions
	static FbxScene* ImportFbxScene(const std::filesystem::path& FilePath);
	static FbxMesh* FindFirstMesh(FbxNode* RootNode, FbxNode** OutNode);
	static void ExtractVertices(FbxMesh* Mesh, FFbxMeshInfo* OutMeshInfo, const Configuration& Config);
	static void ExtractMaterials(FbxNode* Node, const std::filesystem::path& FbxFilePath, FFbxMeshInfo* OutMeshInfo);
	static std::filesystem::path ResolveTexturePath(const std::string& OriginalPath, const std::filesystem::path& FbxDirectory, const std::filesystem::path& FbxFilePath);
	static void ExtractGeometryData(FbxMesh* Mesh, FFbxMeshInfo* OutMeshInfo, const Configuration& Config);
	static void BuildMeshSections(const TArray<TArray<uint32>>& IndicesPerMaterial, FFbxMeshInfo* OutMeshInfo);

	static inline FbxManager* SdkManager = nullptr;
	static inline FbxIOSettings* IoSettings = nullptr;
};
