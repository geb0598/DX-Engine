#include "pch.h"

#include "Manager/Asset/Public/FbxManager.h"
#include "Manager/Asset/Public/AssetManager.h"

// ========================================
// 🔸 Public API
// ========================================

FStaticMesh* FFbxManager::LoadFbxStaticMeshAsset(const FName& FilePath, const FFbxImporter::Configuration& Config)
{
	FFbxStaticMeshInfo MeshInfo;
	if (!FFbxImporter::LoadStaticMesh(FilePath.ToString(), &MeshInfo, Config))
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

	UStaticMesh* StaticMesh = NewObject<UStaticMesh>();
	StaticMesh->SetStaticMeshAsset(StaticMeshAsset);

	// Materials 생성 및 설정
	for (int32 i = 0; i < StaticMeshAsset->MaterialInfo.Num(); ++i)
	{
		// MaterialInfo를 복사해서 전달 (참조 문제 회피)
		FMaterial MaterialCopy = StaticMeshAsset->MaterialInfo[i];
		UMaterial* NewMaterial = CreateMaterialFromInfo(MaterialCopy, i);
		StaticMesh->SetMaterial(i, NewMaterial);
	}

	// 캐시에 등록
	UAssetManager::GetInstance().AddStaticMeshToCache(FilePath, StaticMesh);

	return StaticMesh;
}

// ========================================
// 🔸 Private Helper Functions
// ========================================

void FFbxManager::ConvertFbxToStaticMesh(const FFbxStaticMeshInfo& MeshInfo, FStaticMesh* OutStaticMesh)
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
		FMaterial Material{};
		Material.Name = FbxMat.MaterialName;
		Material.Kd = FVector(0.9f, 0.9f, 0.9f);
		Material.Ka = FVector(0.2f, 0.2f, 0.2f);
		Material.Ks = FVector(0.5f, 0.5f, 0.5f);
		Material.Ns = 32.0f;
		Material.D = 1.0f;

		if (!FbxMat.DiffuseTexturePath.empty())
		{
			Material.KdMap = FbxMat.DiffuseTexturePath.generic_string();
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
	UMaterial* NewMaterial = NewObject<UMaterial>();
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
			UE_LOG_SUCCESS("[FbxManager] Material %d - Texture Loaded Successfully", MaterialIndex);
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

// ========================================
// 🔸 Skeletal Mesh Public API
// ========================================

USkeletalMesh* FFbxManager::LoadFbxSkeletalMesh(const FName& FilePath, const FFbxImporter::Configuration& Config)
{
	FFbxSkeletalMeshInfo SkeletalMeshInfo;
	if (!FFbxImporter::LoadSkeletalMesh(FilePath.ToString(), &SkeletalMeshInfo, Config))
	{
		UE_LOG_ERROR("FBX 스켈레탈 메시 로드 실패: %s", FilePath.ToString().c_str());
		return nullptr;
	}

	USkeletalMesh* SkeletalMesh = NewObject<USkeletalMesh>();

	if (!ConvertFbxToSkeletalMesh(SkeletalMeshInfo, SkeletalMesh))
	{
		UE_LOG_ERROR("FBX → SkeletalMesh 변환 실패: %s", FilePath.ToString().c_str());
		delete SkeletalMesh;
		return nullptr;
	}

	UE_LOG_SUCCESS("FBX SkeletalMesh 변환 완료: %s", FilePath.ToString().c_str());
	return SkeletalMesh;
}

// ========================================
// 🔸 Skeletal Mesh Helper Functions
// ========================================

bool FFbxManager::ConvertFbxToSkeletalMesh(const FFbxSkeletalMeshInfo& FbxData, USkeletalMesh* OutSkeletalMesh)
{
	if (!OutSkeletalMesh)
	{
		UE_LOG_ERROR("유효하지 않은 SkeletalMesh입니다.");
		return false;
	}

	// 1. 스켈레톤 변환
	ConvertSkeleton(FbxData.Bones, OutSkeletalMesh->GetRefSkeleton());

	// 2. 렌더 데이터 생성 및 변환
	FSkeletalMeshRenderData* RenderData = OutSkeletalMesh->GetSkeletalMeshRenderData();
	if (!RenderData)
	{
		// RenderData가 없으면 새로 생성 (필요시)
		UE_LOG_ERROR("SkeletalMeshRenderData가 없습니다.");
		return false;
	}

	ConvertRenderData(FbxData, RenderData);

	// 3. 스킨 가중치 변환
	ConvertSkinWeights(FbxData.SkinWeights, RenderData->SkinWeightVertices);

	// 4. Inverse Reference Matrices 계산
	OutSkeletalMesh->CalculateInvRefMatrices();

	UE_LOG_SUCCESS("[FbxManager] SkeletalMesh 변환 완료 - Bones: %d, Vertices: %d",
		FbxData.Bones.Num(), FbxData.VertexList.Num());

	return true;
}

void FFbxManager::ConvertSkeleton(const TArray<FFbxBoneInfo>& FbxBones, FReferenceSkeleton& OutRefSkeleton)
{
	TArray<FMeshBoneInfo> BoneInfos;
	TArray<FTransform> BonePoses;

	BoneInfos.Reserve(FbxBones.Num());
	BonePoses.Reserve(FbxBones.Num());

	for (int32 i = 0; i < FbxBones.Num(); ++i)
	{
		const FFbxBoneInfo& FbxBone = FbxBones[i];

		// FMeshBoneInfo 생성
		FMeshBoneInfo BoneInfo;
		BoneInfo.Name = FName(FbxBone.BoneName);
		BoneInfo.ParentIndex = FbxBone.ParentIndex;

		// FTransform은 그대로 사용
		FTransform BonePose = FbxBone.LocalTransform;

		BoneInfos.Add(BoneInfo);
		BonePoses.Add(BonePose);

		UE_LOG("[FbxManager] 본 %d 준비: %s (부모: %d)",
			i, FbxBone.BoneName.c_str(), FbxBone.ParentIndex);
	}

	// 한 번에 초기화
	OutRefSkeleton.InitializeFromData(BoneInfos, BonePoses);
	UE_LOG_SUCCESS("[FbxManager] ReferenceSkeleton 초기화 완료: %d 본", BoneInfos.Num());
}

void FFbxManager::ConvertSkinWeights(const TArray<FFbxBoneInfluence>& FbxWeights, TArray<FRawSkinWeight>& OutSkinWeights)
{
	OutSkinWeights.Reset(FbxWeights.Num());

	for (const FFbxBoneInfluence& FbxWeight : FbxWeights)
	{
		FRawSkinWeight SkinWeight;

		// FBX 가중치 → 엔진 가중치
		for (int32 i = 0; i < FFbxBoneInfluence::MAX_INFLUENCES; ++i)
		{
			SkinWeight.InfluenceBones[i] = FbxWeight.BoneIndices[i];
			SkinWeight.InfluenceWeights[i] = FbxWeight.BoneWeights[i];
		}

		OutSkinWeights.Add(SkinWeight);
	}

	UE_LOG("[FbxManager] 스킨 가중치 변환 완료: %d 정점", OutSkinWeights.Num());
}

void FFbxManager::ConvertRenderData(const FFbxSkeletalMeshInfo& FbxData, FSkeletalMeshRenderData* OutRenderData)
{
	if (!OutRenderData)
	{
		UE_LOG_ERROR("유효하지 않은 RenderData입니다.");
		return;
	}

	// StaticMesh 데이터 설정 (지오메트리)
	FStaticMesh& StaticMesh = OutRenderData->StaticMesh;

	// Vertices 변환
	for (int i = 0; i < FbxData.VertexList.Num(); ++i)
	{
		FNormalVertex Vertex{};
		Vertex.Position = FbxData.VertexList[i];
		Vertex.Normal = FbxData.NormalList.IsValidIndex(i) ? FbxData.NormalList[i] : FVector(0, 1, 0);
		Vertex.TexCoord = FbxData.TexCoordList.IsValidIndex(i) ? FbxData.TexCoordList[i] : FVector2(0, 0);
		StaticMesh.Vertices.Add(Vertex);
	}

	StaticMesh.Indices = FbxData.Indices;

	// Materials 변환
	for (const FFbxMaterialInfo& FbxMat : FbxData.Materials)
	{
		FMaterial Material{};
		Material.Name = FbxMat.MaterialName;
		Material.Kd = FVector(0.9f, 0.9f, 0.9f);
		Material.Ka = FVector(0.2f, 0.2f, 0.2f);
		Material.Ks = FVector(0.5f, 0.5f, 0.5f);
		Material.Ns = 32.0f;
		Material.D = 1.0f;

		if (!FbxMat.DiffuseTexturePath.empty())
		{
			Material.KdMap = FbxMat.DiffuseTexturePath.generic_string();
		}
		StaticMesh.MaterialInfo.Add(Material);
	}

	// Sections 변환
	for (const FFbxMeshSection& FbxSection : FbxData.Sections)
	{
		FMeshSection Section{};
		Section.StartIndex = FbxSection.StartIndex;
		Section.IndexCount = FbxSection.IndexCount;
		Section.MaterialSlot = FbxSection.MaterialIndex;
		StaticMesh.Sections.Add(Section);
	}

	// Render Sections 설정
	OutRenderData->RenderSections.Reset();
	for (const FFbxMeshSection& FbxSection : FbxData.Sections)
	{
		FSkeletalMeshRenderSection RenderSection;
		RenderSection.MaterialIndex = FbxSection.MaterialIndex;
		RenderSection.NumTriangles = FbxSection.IndexCount / 3;
		RenderSection.BaseVertexIndex = FbxSection.StartIndex;
		RenderSection.NumVertices = FbxSection.IndexCount; // 단순화
		RenderSection.MaxBoneInfluences = 4; // 기본값

		OutRenderData->RenderSections.Add(RenderSection);
	}

	UE_LOG("[FbxManager] RenderData 변환 완료 - Vertices: %d, Indices: %d, Sections: %d",
		StaticMesh.Vertices.Num(), StaticMesh.Indices.Num(), StaticMesh.Sections.Num());
}
