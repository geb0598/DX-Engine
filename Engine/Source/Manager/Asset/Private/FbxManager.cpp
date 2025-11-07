#include "pch.h"

#include "Manager/Asset/Public/FbxManager.h"
#include "Manager/Asset/Public/AssetManager.h"

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

	// FBX 데이터를 FStaticMesh로 변환
	for (int i = 0; i < MeshInfo.VertexList.Num(); ++i)
	{
		FNormalVertex Vertex{};
		Vertex.Position = MeshInfo.VertexList[i];
		Vertex.Normal = MeshInfo.NormalList.IsValidIndex(i) ? MeshInfo.NormalList[i] : FVector(0, 1, 0);
		Vertex.TexCoord = MeshInfo.TexCoordList.IsValidIndex(i) ? MeshInfo.TexCoordList[i] : FVector2(0, 0);
		StaticMesh->Vertices.Add(Vertex);
	}

	StaticMesh->Indices = MeshInfo.Indices;

	FMaterial Material;
	Material.Name = MeshInfo.MaterialName;
	if (!MeshInfo.DiffuseTexturePath.empty())
	{
		Material.KdMap = MeshInfo.DiffuseTexturePath.string();
	}
	
	StaticMesh->MaterialInfo.Add(Material);

	FMeshSection Section{};
	Section.StartIndex = 0;
	Section.IndexCount = static_cast<uint32>(StaticMesh->Indices.Num());
	Section.MaterialSlot = 0;
	StaticMesh->Sections.Add(Section);

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

	for (int32 i = 0; i < StaticMeshAsset->MaterialInfo.Num(); ++i)
	{
		const FMaterial& MaterialInfo = StaticMeshAsset->MaterialInfo[i];
		UMaterial* NewMaterial = new UMaterial();
		NewMaterial->SetMaterialData(MaterialInfo);

		if (!MaterialInfo.KdMap.empty())
		{
			UTexture* DiffuseTexture = UAssetManager::GetInstance().LoadTexture(FName(MaterialInfo.KdMap));
			NewMaterial->SetDiffuseTexture(DiffuseTexture);
		}
		StaticMesh->SetMaterial(i, NewMaterial);
	}

	// 캐시에 등록
	UAssetManager::GetInstance().AddStaticMeshToCache(FilePath, StaticMesh);

	return StaticMesh;
}
