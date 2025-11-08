#pragma once
#include "FbxImporter.h"
#include "Component/Mesh/Public/StaticMesh.h"
#include "Runtime/Engine/Public/SkeletalMesh.h"
#include "Runtime/Engine/Public/ReferenceSkeleton.h"

class FFbxManager
{
public:
	// ========================================
	// 🔸 Static Mesh
	// ========================================

	static FStaticMesh* LoadFbxStaticMeshAsset(
		const FName& FilePath,
		const FFbxImporter::Configuration& Config = {});

	static UStaticMesh* LoadFbxStaticMesh(
		const FName& FilePath,
		const FFbxImporter::Configuration& Config = {});

	// ========================================
	// 🔸 Skeletal Mesh
	// ========================================

	static USkeletalMesh* LoadFbxSkeletalMesh(
		const FName& FilePath,
		const FFbxImporter::Configuration& Config = {});

private:
	// ========================================
	// 🔸 Static Mesh Helpers
	// ========================================

	static void ConvertFbxToStaticMesh(const FFbxStaticMeshInfo& MeshInfo, FStaticMesh* OutStaticMesh);
	static UMaterial* CreateMaterialFromInfo(const FMaterial& MaterialInfo, int32 MaterialIndex);

	// ========================================
	// 🔸 Skeletal Mesh Helpers
	// ========================================

	/** FFbxSkeletalMeshInfo를 USkeletalMesh로 변환 */
	static bool ConvertFbxToSkeletalMesh(
		const FFbxSkeletalMeshInfo& FbxData,
		USkeletalMesh* OutSkeletalMesh);

	/** FFbxBoneInfo 배열을 FReferenceSkeleton으로 변환 */
	static void ConvertSkeleton(
		const TArray<FFbxBoneInfo>& FbxBones,
		FReferenceSkeleton& OutRefSkeleton);

	/** FFbxBoneInfluence 배열을 FRawSkinWeight 배열로 변환 */
	static void ConvertSkinWeights(
		const TArray<FFbxBoneInfluence>& FbxWeights,
		TArray<FRawSkinWeight>& OutSkinWeights);

	/** FFbxSkeletalMeshInfo를 FSkeletalMeshRenderData로 변환 */
	static void ConvertRenderData(
		const FFbxSkeletalMeshInfo& FbxData,
		FSkeletalMeshRenderData* OutRenderData);
};
