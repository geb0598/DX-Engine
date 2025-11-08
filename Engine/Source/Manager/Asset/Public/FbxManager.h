#pragma once
#include "FbxImporter.h"
#include "Component/Mesh/Public/StaticMesh.h"
#include "Runtime/Engine/Public/SkeletalMesh.h"
#include "Runtime/Engine/Public/ReferenceSkeleton.h"

class UObject; // Forward declaration

class FFbxManager
{
public:
	// ========================================
	// 🔸 통합 메시 로드 (자동 타입 판단)
	// ========================================

	/**
	 * FBX 파일을 로드하여 자동으로 Static/Skeletal 타입을 판단하여 반환
	 * @return UStaticMesh 또는 USkeletalMesh (UObject*로 반환, 캐스팅 필요)
	 */
	static UObject* LoadFbxMesh(
		const FName& FilePath,
		const FFbxImporter::Configuration& Config = {});

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

	/** FFbxSkeletalMeshInfo를 FStaticMesh로 변환 (지오메트리 데이터만) */
	static void ConvertFbxSkeletalToStaticMesh(
		const FFbxSkeletalMeshInfo& FbxData,
		FStaticMesh* OutStaticMesh);
};
