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

struct FFbxStaticMeshInfo
{
	TArray<FVector> VertexList;
	TArray<FVector> NormalList;
	TArray<FVector2> TexCoordList;
	TArray<uint32> Indices;

	TArray<FFbxMaterialInfo> Materials;
	TArray<FFbxMeshSection> Sections;
};

// ========================================
// 🔸 스켈레탈 메시 전용 구조체
// ========================================

/** FBX에서 추출한 본 정보 (엔진 독립적) */
struct FFbxBoneInfo
{
	std::string BoneName;
	int32 ParentIndex;  // -1이면 루트
	FTransform LocalTransform;  // 부모 기준 로컬 변환

	FFbxBoneInfo()
		: BoneName("")
		, ParentIndex(-1)
		, LocalTransform()
	{}
};

/** FBX에서 추출한 본 영향력 정보 (엔진 독립적) */
struct FFbxBoneInfluence
{
	static constexpr uint32 MAX_INFLUENCES = 12;

	/** 영향을 주는 본의 인덱스들 */
	int32 BoneIndices[MAX_INFLUENCES];

	/** 각 본의 가중치 (0~255, 합이 255) */
	uint8 BoneWeights[MAX_INFLUENCES];

	FFbxBoneInfluence()
	{
		for (int i = 0; i < MAX_INFLUENCES; ++i)
		{
			BoneIndices[i] = -1;
			BoneWeights[i] = 0;
		}
	}
};

/** 스켈레탈 메시 전용 데이터 */
struct FFbxSkeletalMeshInfo
{
	FName PathFileName;

	// 지오메트리 데이터 (스태틱과 동일)
	TArray<FVector> VertexList;
	TArray<FVector> NormalList;
	TArray<FVector2> TexCoordList;
	TArray<uint32> Indices;

	TArray<FFbxMaterialInfo> Materials;
	TArray<FFbxMeshSection> Sections;

	// 스켈레탈 전용 데이터 (FBX 전용 타입 사용)
	TArray<FFbxBoneInfo> Bones;              // 본 계층 구조
	TArray<FFbxBoneInfluence> SkinWeights;   // 정점별 스킨 가중치 (VertexList와 1:1 대응)
	TArray<int32> ControlPointIndices;       // 각 PolygonVertex가 어떤 ControlPoint에서 왔는지 매핑 (VertexList와 1:1 대응)
};

enum class EFbxMeshType
{
	Static,
	Skeletal,
	Unknown
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

	// 🔸 Public API - 타입별 로드 함수

	/** FBX 파일에서 메시 타입 판단 */
	static EFbxMeshType DetermineMeshType(const std::filesystem::path& FilePath);

	/** 스태틱 메시 임포트 */
	static bool LoadStaticMesh(
		const std::filesystem::path& FilePath,
		FFbxStaticMeshInfo* OutMeshInfo,
		Configuration Config = {});

	/** 스켈레탈 메시 임포트 */
	static bool LoadSkeletalMesh(
		const std::filesystem::path& FilePath,
		FFbxSkeletalMeshInfo* OutMeshInfo,
		Configuration Config = {});

private:
	// Helper functions
	static FbxScene* ImportFbxScene(const std::filesystem::path& FilePath);
	static FbxMesh* FindFirstMesh(FbxNode* RootNode, FbxNode** OutNode);
	static void ExtractVertices(FbxMesh* Mesh, FFbxStaticMeshInfo* OutMeshInfo, const Configuration& Config);
	static void ExtractMaterials(FbxNode* Node, const std::filesystem::path& FbxFilePath, FFbxStaticMeshInfo* OutMeshInfo);
	static std::filesystem::path ResolveTexturePath(const std::string& OriginalPath, const std::filesystem::path& FbxDirectory, const std::filesystem::path& FbxFilePath);
	static void ExtractGeometryData(FbxMesh* Mesh, FFbxStaticMeshInfo* OutMeshInfo, const Configuration& Config);
	static void BuildMeshSections(const TArray<TArray<uint32>>& IndicesPerMaterial, FFbxStaticMeshInfo* OutMeshInfo);

	// Skeletal Mesh Helpers
	static FbxMesh* FindFirstSkinnedMesh(FbxNode* RootNode, FbxNode** OutNode);
	static bool ExtractSkeleton(FbxScene* Scene, FbxMesh* Mesh, FFbxSkeletalMeshInfo* OutMeshInfo);
	static bool ExtractSkinWeights(FbxMesh* Mesh, FFbxSkeletalMeshInfo* OutMeshInfo);
	static void ExtractSkeletalGeometryData(FbxMesh* Mesh, FFbxSkeletalMeshInfo* OutMeshInfo, const Configuration& Config);
	static void ExtractSkeletalMaterials(FbxNode* Node, const std::filesystem::path& FbxFilePath, FFbxSkeletalMeshInfo* OutMeshInfo);
	static void BuildSkeletalMeshSections(const TArray<TArray<uint32>>& IndicesPerMaterial, FFbxSkeletalMeshInfo* OutMeshInfo);

	static inline FbxManager* SdkManager = nullptr;
	static inline FbxIOSettings* IoSettings = nullptr;
};
