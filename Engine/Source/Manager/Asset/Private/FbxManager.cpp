#include "pch.h"

#include "Manager/Asset/Public/FbxManager.h"
#include "Manager/Asset/Public/AssetManager.h"

// ========================================
// 🔸 Public API
// ========================================

FStaticMesh* FFbxManager::LoadFbxStaticMeshAsset(const FName& FilePath, const FFbxImporter::Configuration& Config)
{
	FFbxMeshInfo MeshInfo;
	if (!FFbxImporter::LoadFBX(FilePath.ToString(), &MeshInfo, Config))
	{
		UE_LOG_ERROR("FBX 로드 실패: %s", FilePath.ToString().c_str());
		return nullptr;
	}

	auto StaticMesh = std::make_unique<FStaticMesh>();
	StaticMesh->PathFileName = FilePath;

	ConvertFbxToStaticMesh(MeshInfo, StaticMesh.get());

	UE_LOG_SUCCESS("FBX StaticMesh 변환 완료: %s", FilePath.ToString().c_str());
	return StaticMesh.release();
}

UStaticMesh* FFbxManager::LoadFbxStaticMesh(const FName& FilePath, const FFbxImporter::Configuration& Config)
{
	FStaticMesh* StaticMeshAsset = LoadFbxStaticMeshAsset(FilePath, Config);
	if (!StaticMeshAsset)
		return nullptr;

	UStaticMesh* StaticMesh = new UStaticMesh();
	StaticMesh->SetStaticMeshAsset(StaticMeshAsset);

	// Materials 생성 및 설정
	for (int32 i = 0; i < StaticMeshAsset->MaterialInfo.Num(); ++i)
	{
		UMaterial* NewMaterial = CreateMaterialFromInfo(StaticMeshAsset->MaterialInfo[i], i);
		StaticMesh->SetMaterial(i, NewMaterial);
	}

	// 캐시에 등록
	UAssetManager::GetInstance().AddStaticMeshToCache(FilePath, StaticMesh);

	return StaticMesh;
}

// ========================================
// 🔸 Private Helper Functions
// ========================================

void FFbxManager::ConvertFbxToStaticMesh(const FFbxMeshInfo& MeshInfo, FStaticMesh* OutStaticMesh)
{
	// Vertices 변환
	for (int i = 0; i < MeshInfo.VertexList.Num(); ++i)
	{
		FNormalVertex Vertex{};
		Vertex.Position = MeshInfo.VertexList[i];
		Vertex.Normal = MeshInfo.NormalList.IsValidIndex(i) ? MeshInfo.NormalList[i] : FVector(0, 1, 0);
		Vertex.TexCoord = MeshInfo.TexCoordList.IsValidIndex(i) ? MeshInfo.TexCoordList[i] : FVector2(0, 0);
		OutStaticMesh->Vertices.Add(Vertex);
	}

	OutStaticMesh->Indices = MeshInfo.Indices;

	// Materials 변환
	for (const FFbxMaterialInfo& FbxMat : MeshInfo.Materials)
	{
		FMaterial Material;
		Material.Name = FbxMat.MaterialName;
		if (!FbxMat.DiffuseTexturePath.empty())
		{
			Material.KdMap = FbxMat.DiffuseTexturePath.string();
		}
		OutStaticMesh->MaterialInfo.Add(Material);
	}

	// Sections 변환
	for (const FFbxMeshSection& FbxSection : MeshInfo.Sections)
	{
		FMeshSection Section{};
		Section.StartIndex = FbxSection.StartIndex;
		Section.IndexCount = FbxSection.IndexCount;
		Section.MaterialSlot = FbxSection.MaterialIndex;
		OutStaticMesh->Sections.Add(Section);
	}
}

UMaterial* FFbxManager::CreateMaterialFromInfo(const FMaterial& MaterialInfo, int32 MaterialIndex)
{
	UMaterial* NewMaterial = new UMaterial();
	NewMaterial->SetName(FName(MaterialInfo.Name));
	NewMaterial->SetMaterialData(MaterialInfo);

	// Diffuse Texture 로드
	if (!MaterialInfo.KdMap.empty())
	{
		UE_LOG("[FbxManager] Material %d - Texture Path: %s", MaterialIndex, MaterialInfo.KdMap.c_str());

		UTexture* DiffuseTexture = UAssetManager::GetInstance().LoadTexture(FName(MaterialInfo.KdMap));
		if (DiffuseTexture)
		{
			NewMaterial->SetDiffuseTexture(DiffuseTexture);
		}
		else
		{
			UE_LOG_ERROR("[FbxManager] Material %d - Texture Load Failed: %s", MaterialIndex, MaterialInfo.KdMap.c_str());
		}
	}
	else
	{
		UE_LOG_WARNING("[FbxManager] Material %d - No Texture Path", MaterialIndex);
	}

	return NewMaterial;
}
