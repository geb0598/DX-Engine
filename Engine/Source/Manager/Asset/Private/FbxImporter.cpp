#include "pch.h"
#include "Source/Manager/Asset/Public/FbxImporter.h"

// ========================================
// 🔸 Public API
// ========================================

bool FFbxImporter::Initialize()
{
	if (SdkManager) { return true; }

	SdkManager = FbxManager::Create();
	IoSettings = FbxIOSettings::Create(SdkManager, IOSROOT);
	SdkManager->SetIOSettings(IoSettings);

	UE_LOG_SUCCESS("FBX SDK Initialized.");
	return true;
}

void FFbxImporter::Shutdown()
{
	if (!SdkManager) { return; }

	IoSettings->Destroy();
	SdkManager->Destroy();
	SdkManager = nullptr;
	IoSettings = nullptr;

	UE_LOG_SUCCESS("FBX SDK Shut down.");
}

bool FFbxImporter::LoadStaticMesh(const std::filesystem::path& FilePath, FFbxStaticMeshInfo* OutMeshInfo, Configuration Config)
{
	// 입력 검증
	if (!OutMeshInfo)
	{
		UE_LOG_ERROR("유효하지 않은 FBXStaticMeshInfo입니다.");
		return false;
	}

	if (!SdkManager)
	{
		UE_LOG_ERROR("FBX SDK Manager가 존재하지 않습니다.");
		return false;
	}

	if (!std::filesystem::exists(FilePath))
	{
		UE_LOG_ERROR("FBX 파일이 존재하지 않습니다: %s", FilePath.string().c_str());
		return false;
	}

	// FBX Scene 임포트
	FbxScene* Scene = ImportFbxScene(FilePath);
	if (!Scene)
	{
		return false;
	}

	// 첫 번째 메시 찾기
	FbxNode* RootNode = Scene->GetRootNode();
	if (!RootNode)
	{
		UE_LOG_ERROR("FBX 파일을 탐색을 실패했습니다.");
		Scene->Destroy();
		return false;
	}

	FbxNode* MeshNode = nullptr;
	FbxMesh* Mesh = FindFirstMesh(RootNode, &MeshNode);
	if (!Mesh || !MeshNode)
	{
		UE_LOG_ERROR("FBX에 유효한 메시가 없습니다");
		Scene->Destroy();
		return false;
	}

	// 데이터 추출
	ExtractVertices(Mesh, OutMeshInfo, Config);
	ExtractMaterials(MeshNode, FilePath, OutMeshInfo);
	ExtractGeometryData(Mesh, OutMeshInfo, Config);

	Scene->Destroy();
	return true;
}

// ========================================
// 🔸 Private Helper Functions
// ========================================

FbxScene* FFbxImporter::ImportFbxScene(const std::filesystem::path& FilePath)
{
	FbxImporter* Importer = FbxImporter::Create(SdkManager, "");
	if (!Importer->Initialize(FilePath.string().c_str(), -1, IoSettings))
	{
		UE_LOG_ERROR("FBX 파일을 로드 실패했습니다: %s", FilePath.string().c_str());
		Importer->Destroy();
		return nullptr;
	}

	FbxScene* Scene = FbxScene::Create(SdkManager, "Scene");
	Importer->Import(Scene);
	Importer->Destroy();

	// 모든 지오메트리를 삼각형으로 변환
	FbxGeometryConverter GeomConverter(SdkManager);
	GeomConverter.Triangulate(Scene, true);

	return Scene;
}

FbxMesh* FFbxImporter::FindFirstMesh(FbxNode* RootNode, FbxNode** OutNode)
{
	for (int i = 0; i < RootNode->GetChildCount(); ++i)
	{
		FbxNode* Child = RootNode->GetChild(i);
		if (FbxMesh* Mesh = Child->GetMesh())
		{
			*OutNode = Child;
			return Mesh;
		}
	}
	return nullptr;
}

void FFbxImporter::ExtractVertices(FbxMesh* Mesh, FFbxStaticMeshInfo* OutMeshInfo, const Configuration& Config)
{
	const int ControlPointCount = Mesh->GetControlPointsCount();
	FbxVector4* ControlPoints = Mesh->GetControlPoints();

	OutMeshInfo->VertexList.Reserve(ControlPointCount);
	for (int i = 0; i < ControlPointCount; ++i)
	{
		FVector Pos(ControlPoints[i][0], ControlPoints[i][1], ControlPoints[i][2]);
		if (Config.bConvertToUEBasis)
		{
			Pos = FVector(Pos.X, -Pos.Y, Pos.Z);
		}
		OutMeshInfo->VertexList.Add(Pos);
	}
}

void FFbxImporter::ExtractMaterials(FbxNode* Node, const std::filesystem::path& FbxFilePath, FFbxStaticMeshInfo* OutMeshInfo)
{
	const int MaterialCount = Node->GetMaterialCount();
	UE_LOG("[FbxImporter] Material Count: %d", MaterialCount);

	for (int m = 0; m < MaterialCount; ++m)
	{
		FbxSurfaceMaterial* Material = Node->GetMaterial(m);
		if (!Material) continue;

		FFbxMaterialInfo MatInfo;
		const char* MaterialName = Material->GetName();
		MatInfo.MaterialName = (MaterialName && strlen(MaterialName) > 0)
			? MaterialName
			: "Material_" + std::to_string(m);

		UE_LOG("[FbxImporter] Material %d: %s", m, MatInfo.MaterialName.c_str());

		// Diffuse 텍스처 추출
		if (FbxProperty Prop = Material->FindProperty(FbxSurfaceMaterial::sDiffuse); Prop.IsValid())
		{
			int LayeredTextureCount = Prop.GetSrcObjectCount<FbxLayeredTexture>();
			if (LayeredTextureCount > 0)
			{
				UE_LOG_WARNING("[FbxImporter] Layered Texture는 아직 지원하지 않습니다.");
			}
			else
			{
				int TextureCount = Prop.GetSrcObjectCount<FbxFileTexture>();
				if (TextureCount > 0)
				{
					if (FbxFileTexture* Texture = Prop.GetSrcObject<FbxFileTexture>(0))
					{
						std::string OriginalTexturePath = Texture->GetFileName();
						UE_LOG("[FbxImporter] Material %d - Texture Path: %s", m, OriginalTexturePath.c_str());

						std::filesystem::path FbxDirectory = FbxFilePath.parent_path();
						std::filesystem::path ResolvedPath = ResolveTexturePath(OriginalTexturePath, FbxDirectory, FbxFilePath);

						if (!ResolvedPath.empty())
						{
							MatInfo.DiffuseTexturePath = ResolvedPath;
						}
					}
				}
			}
		}

		OutMeshInfo->Materials.Add(MatInfo);
	}

	// Material이 없으면 기본 Material 추가
	if (OutMeshInfo->Materials.Num() == 0)
	{
		FFbxMaterialInfo DefaultMat;
		DefaultMat.MaterialName = "Default";
		OutMeshInfo->Materials.Add(DefaultMat);
		UE_LOG_WARNING("[FbxImporter] Material이 없어 기본 Material을 추가했습니다.");
	}
}

std::filesystem::path FFbxImporter::ResolveTexturePath(
	const std::string& OriginalPath,
	const std::filesystem::path& FbxDirectory,
	const std::filesystem::path& FbxFilePath)
{
	std::filesystem::path OriginalFsPath(OriginalPath);

	// 방법 1: 원본 경로가 유효한지 확인
	if (std::filesystem::exists(OriginalFsPath))
	{
		UE_LOG_SUCCESS("[FbxImporter] 텍스처 찾음 (원본 경로): %s", OriginalFsPath.string().c_str());
		return OriginalFsPath;
	}

	// 방법 2: FBX 파일과 같은 디렉토리에서 파일명만으로 찾기
	std::filesystem::path FilenameOnly = OriginalFsPath.filename();
	std::filesystem::path LocalTexturePath = FbxDirectory / FilenameOnly;

	if (std::filesystem::exists(LocalTexturePath))
	{
		UE_LOG_SUCCESS("[FbxImporter] 텍스처 찾음 (FBX 디렉토리): %s", LocalTexturePath.string().c_str());
		return LocalTexturePath;
	}

	// 방법 3: .fbm 폴더에서 찾기 (FBX SDK 기본 텍스처 저장 위치)
	std::filesystem::path FbxFilename = FbxFilePath.stem();
	std::filesystem::path FbmFolder = FbxDirectory / (FbxFilename.string() + ".fbm");
	std::filesystem::path FbmTexturePath = FbmFolder / FilenameOnly;

	if (std::filesystem::exists(FbmTexturePath))
	{
		UE_LOG_SUCCESS("[FbxImporter] 텍스처 찾음 (.fbm 폴더): %s", FbmTexturePath.string().c_str());
		return FbmTexturePath;
	}

	UE_LOG_WARNING("[FbxImporter] 텍스처를 찾을 수 없습니다: %s", OriginalPath.c_str());
	UE_LOG_WARNING("[FbxImporter] 시도한 경로: %s", FbmTexturePath.string().c_str());
	return {};
}

void FFbxImporter::ExtractGeometryData(
	FbxMesh* Mesh,
	FFbxStaticMeshInfo* OutMeshInfo,
	const Configuration& Config)
{
	// Material Mapping 정보 가져오기
	FbxLayerElementMaterial* MaterialElement = Mesh->GetElementMaterial();
	FbxGeometryElement::EMappingMode MaterialMappingMode = FbxGeometryElement::eNone;
	if (MaterialElement)
	{
		MaterialMappingMode = MaterialElement->GetMappingMode();
		UE_LOG("[FbxImporter] Material Mapping Mode: %d", (int)MaterialMappingMode);
	}

	// 기존 ControlPoint 기반 VertexList를 백업
	TArray<FVector> ControlPointPositions = OutMeshInfo->VertexList;

	// 새로운 Polygon Vertex 기반 데이터로 재구성
	OutMeshInfo->VertexList.Empty();
	OutMeshInfo->NormalList.Empty();
	OutMeshInfo->TexCoordList.Empty();
	OutMeshInfo->Indices.Empty();

	// Material별 인덱스 그룹 초기화
	TArray<TArray<uint32>> IndicesPerMaterial;
	IndicesPerMaterial.Reset(OutMeshInfo->Materials.Num());
	for (int i = 0; i < OutMeshInfo->Materials.Num(); ++i)
	{
		IndicesPerMaterial.Add(TArray<uint32>());
	}

	uint32 VertexCounter = 0;

	// 폴리곤별로 버텍스 데이터 생성
	const int PolygonCount = Mesh->GetPolygonCount();
	for (int p = 0; p < PolygonCount; ++p)
	{
		// 이 Polygon이 사용하는 Material Index 확인
		int MaterialIndex = 0;
		if (MaterialElement)
		{
			switch (MaterialMappingMode)
			{
			case FbxGeometryElement::eByPolygon:
				MaterialIndex = MaterialElement->GetIndexArray().GetAt(p);
				break;
			case FbxGeometryElement::eAllSame:
				MaterialIndex = 0;
				break;
			default:
				MaterialIndex = 0;
				break;
			}
		}

		// Material Index 범위 검증
		if (MaterialIndex < 0 || MaterialIndex >= OutMeshInfo->Materials.Num())
		{
			MaterialIndex = 0;
		}

		// Triangulate를 거쳤기 때문에 PolygonSize는 항상 3
		int PolySize = Mesh->GetPolygonSize(p);
		for (int v = 0; v < PolySize; ++v)
		{
			int CtrlPointIndex = Mesh->GetPolygonVertex(p, v);

			// Position: ControlPoint에서 가져오기
			if (CtrlPointIndex >= 0 && CtrlPointIndex < ControlPointPositions.Num())
			{
				OutMeshInfo->VertexList.Add(ControlPointPositions[CtrlPointIndex]);
			}
			else
			{
				OutMeshInfo->VertexList.Add(FVector(0, 0, 0));
			}

			// Normal 추출
			FbxVector4 Normal;
			if (Mesh->GetPolygonVertexNormal(p, v, Normal))
			{
				FVector N(Normal[0], Normal[1], Normal[2]);
				if (Config.bConvertToUEBasis)
				{
					N = FVector(N.X, -N.Y, N.Z);
				}
				OutMeshInfo->NormalList.Add(N);
			}
			else
			{
				OutMeshInfo->NormalList.Add(FVector(0, 1, 0));
			}

			// UV 추출
			FbxStringList UVSetNames;
			Mesh->GetUVSetNames(UVSetNames);
			if (UVSetNames.GetCount() > 0)
			{
				FbxVector2 UV;
				bool bUnmapped = false;
				if (Mesh->GetPolygonVertexUV(p, v, UVSetNames[0], UV, bUnmapped))
				{
					FVector2 UVConv(UV[0], 1.0f - UV[1]);
					OutMeshInfo->TexCoordList.Add(UVConv);
				}
				else
				{
					OutMeshInfo->TexCoordList.Add(FVector2(0, 0));
				}
			}
			else
			{
				OutMeshInfo->TexCoordList.Add(FVector2(0, 0));
			}

			// 인덱스는 순차적으로
			OutMeshInfo->Indices.Add(VertexCounter);
			IndicesPerMaterial[MaterialIndex].Add(VertexCounter);
			VertexCounter++;
		}
	}

	UE_LOG("[FbxImporter] Total Vertices: %d, Normals: %d, UVs: %d, Indices: %d",
		OutMeshInfo->VertexList.Num(), OutMeshInfo->NormalList.Num(),
		OutMeshInfo->TexCoordList.Num(), OutMeshInfo->Indices.Num());

	// Mesh Section 정보 생성
	BuildMeshSections(IndicesPerMaterial, OutMeshInfo);
}

void FFbxImporter::BuildMeshSections(const TArray<TArray<uint32>>& IndicesPerMaterial, FFbxStaticMeshInfo* OutMeshInfo)
{
	uint32 CurrentIndexOffset = 0;
	for (int i = 0; i < IndicesPerMaterial.Num(); ++i)
	{
		if (IndicesPerMaterial[i].Num() > 0)
		{
			FFbxMeshSection Section;
			Section.StartIndex = CurrentIndexOffset;
			Section.IndexCount = IndicesPerMaterial[i].Num();
			Section.MaterialIndex = i;
			OutMeshInfo->Sections.Add(Section);

			UE_LOG("[FbxImporter] Section %d: StartIndex=%d, Count=%d, MaterialIndex=%d",
				i, Section.StartIndex, Section.IndexCount, Section.MaterialIndex);

			CurrentIndexOffset += Section.IndexCount;
		}
	}
}

// ========================================
// 🔸 Skeletal Mesh Implementation
// ========================================

bool FFbxImporter::LoadSkeletalMesh(const std::filesystem::path& FilePath, FFbxSkeletalMeshInfo* OutMeshInfo, Configuration Config)
{
	// 입력 검증
	if (!OutMeshInfo)
	{
		UE_LOG_ERROR("유효하지 않은 FFbxSkeletalMeshInfo입니다.");
		return false;
	}

	if (!SdkManager)
	{
		UE_LOG_ERROR("FBX SDK Manager가 존재하지 않습니다.");
		return false;
	}

	if (!std::filesystem::exists(FilePath))
	{
		UE_LOG_ERROR("FBX 파일이 존재하지 않습니다: %s", FilePath.string().c_str());
		return false;
	}

	// FBX Scene 임포트
	FbxScene* Scene = ImportFbxScene(FilePath);
	if (!Scene)
	{
		return false;
	}

	// 첫 번째 스킨 메시 찾기
	FbxNode* RootNode = Scene->GetRootNode();
	if (!RootNode)
	{
		UE_LOG_ERROR("FBX 루트 노드를 찾을 수 없습니다.");
		Scene->Destroy();
		return false;
	}

	FbxNode* MeshNode = nullptr;
	FbxMesh* Mesh = FindFirstSkinnedMesh(RootNode, &MeshNode);
	if (!Mesh || !MeshNode)
	{
		UE_LOG_ERROR("FBX에 유효한 스켈레탈 메시가 없습니다");
		Scene->Destroy();
		return false;
	}

	// 스켈레톤 추출
	if (!ExtractSkeleton(Scene, Mesh, OutMeshInfo))
	{
		UE_LOG_ERROR("스켈레톤 추출 실패");
		Scene->Destroy();
		return false;
	}

	// 지오메트리 데이터 추출
	ExtractSkeletalGeometryData(Mesh, OutMeshInfo, Config);

	// 스킨 가중치 추출
	if (!ExtractSkinWeights(Mesh, OutMeshInfo))
	{
		UE_LOG_ERROR("스킨 가중치 추출 실패");
		Scene->Destroy();
		return false;
	}

	// 머티리얼 추출
	ExtractSkeletalMaterials(MeshNode, FilePath, OutMeshInfo);

	Scene->Destroy();
	UE_LOG_SUCCESS("스켈레탈 메시 로드 완료: %s", FilePath.string().c_str());
	return true;
}

FbxMesh* FFbxImporter::FindFirstSkinnedMesh(FbxNode* RootNode, FbxNode** OutNode)
{
	for (int i = 0; i < RootNode->GetChildCount(); ++i)
	{
		FbxNode* Child = RootNode->GetChild(i);
		if (FbxMesh* Mesh = Child->GetMesh())
		{
			// 스킨 디포머가 있는지 확인
			int DeformerCount = Mesh->GetDeformerCount(FbxDeformer::eSkin);
			if (DeformerCount > 0)
			{
				*OutNode = Child;
				return Mesh;
			}
		}

		// 재귀적으로 자식 노드 탐색
		if (FbxMesh* FoundMesh = FindFirstSkinnedMesh(Child, OutNode))
		{
			return FoundMesh;
		}
	}
	return nullptr;
}

bool FFbxImporter::ExtractSkeleton(FbxScene* Scene, FbxMesh* Mesh, FFbxSkeletalMeshInfo* OutMeshInfo)
{
	// 스킨 디포머 찾기
	FbxSkin* Skin = nullptr;
	int DeformerCount = Mesh->GetDeformerCount(FbxDeformer::eSkin);
	if (DeformerCount == 0)
	{
		UE_LOG_ERROR("메시에 스킨 디포머가 없습니다.");
		return false;
	}

	Skin = (FbxSkin*)Mesh->GetDeformer(0, FbxDeformer::eSkin);
	if (!Skin)
	{
		UE_LOG_ERROR("스킨 디포머를 가져올 수 없습니다.");
		return false;
	}

	int ClusterCount = Skin->GetClusterCount();
	if (ClusterCount == 0)
	{
		UE_LOG_ERROR("본 클러스터가 없습니다.");
		return false;
	}

	UE_LOG("[FbxImporter] 본 개수: %d", ClusterCount);

	// 본 정보를 임시로 저장할 맵 (FbxNode* -> BoneIndex)
	TMap<FbxNode*, int32> BoneNodeToIndexMap;

	// 1차: 모든 본 수집
	for (int i = 0; i < ClusterCount; ++i)
	{
		FbxCluster* Cluster = Skin->GetCluster(i);
		FbxNode* LinkNode = Cluster->GetLink();
		if (!LinkNode)
			continue;

		if (BoneNodeToIndexMap.Find(LinkNode))
			continue; // 이미 추가된 본

		FFbxBoneInfo BoneInfo;
		BoneInfo.BoneName = LinkNode->GetName();
		BoneInfo.ParentIndex = -1; // 나중에 설정

		// 로컬 변환 추출
		FbxAMatrix LocalTransform = LinkNode->EvaluateLocalTransform();
		FbxVector4 T = LocalTransform.GetT();
		FbxQuaternion R = LocalTransform.GetQ();
		FbxVector4 S = LocalTransform.GetS();

		BoneInfo.LocalTransform.Translation = FVector(T[0], T[1], T[2]);
		BoneInfo.LocalTransform.Rotation = FQuaternion(R[0], R[1], R[2], R[3]);
		BoneInfo.LocalTransform.Scale = FVector(S[0], S[1], S[2]);

		int32 BoneIndex = OutMeshInfo->Bones.Num();
		OutMeshInfo->Bones.Add(BoneInfo);
		BoneNodeToIndexMap.Add(LinkNode, BoneIndex);

		UE_LOG("[FbxImporter] 본 %d: %s", BoneIndex, BoneInfo.BoneName.c_str());
	}

	// 2차: 부모 관계 설정
	for (int32 i = 0; i < OutMeshInfo->Bones.Num(); ++i)
	{
		// 본 노드 찾기
		FbxNode* BoneNode = nullptr;
		for (auto& Pair : BoneNodeToIndexMap)
		{
			if (Pair.second == i)
			{
				BoneNode = Pair.first;
				break;
			}
		}

		if (!BoneNode)
			continue;

		FbxNode* ParentNode = BoneNode->GetParent();
		if (ParentNode)
		{
			int32* ParentIndexPtr = BoneNodeToIndexMap.Find(ParentNode);
			if (ParentIndexPtr)
			{
				OutMeshInfo->Bones[i].ParentIndex = *ParentIndexPtr;
			}
		}
	}

	return true;
}

bool FFbxImporter::ExtractSkinWeights(FbxMesh* Mesh, FFbxSkeletalMeshInfo* OutMeshInfo)
{
	FbxSkin* Skin = (FbxSkin*)Mesh->GetDeformer(0, FbxDeformer::eSkin);
	if (!Skin)
	{
		UE_LOG_ERROR("스킨 디포머를 찾을 수 없습니다.");
		return false;
	}

	int VertexCount = OutMeshInfo->VertexList.Num();
	OutMeshInfo->SkinWeights.Reset(VertexCount);

	// 초기화
	for (int i = 0; i < VertexCount; ++i)
	{
		OutMeshInfo->SkinWeights.Add(FFbxBoneInfluence());
	}

	int ClusterCount = Skin->GetClusterCount();

	for (int ClusterIndex = 0; ClusterIndex < ClusterCount; ++ClusterIndex)
	{
		FbxCluster* Cluster = Skin->GetCluster(ClusterIndex);
		int* Indices = Cluster->GetControlPointIndices();
		double* Weights = Cluster->GetControlPointWeights();
		int IndexCount = Cluster->GetControlPointIndicesCount();

		for (int i = 0; i < IndexCount; ++i)
		{
			int VertexIndex = Indices[i];
			double Weight = Weights[i];

			if (VertexIndex >= 0 && VertexIndex < VertexCount && Weight > 0.0001)
			{
				FFbxBoneInfluence& Influence = OutMeshInfo->SkinWeights[VertexIndex];

				// 빈 슬롯 찾기
				for (int j = 0; j < FFbxBoneInfluence::MAX_INFLUENCES; ++j)
				{
					if (Influence.BoneIndices[j] == -1)
					{
						Influence.BoneIndices[j] = ClusterIndex;
						Influence.BoneWeights[j] = static_cast<uint8>(Weight * 255.0);
						break;
					}
				}
			}
		}
	}

	UE_LOG_SUCCESS("[FbxImporter] 스킨 가중치 추출 완료: %d 정점", VertexCount);
	return true;
}

void FFbxImporter::ExtractSkeletalGeometryData(FbxMesh* Mesh, FFbxSkeletalMeshInfo* OutMeshInfo, const Configuration& Config)
{
	// 컨트롤 포인트(정점) 추출
	const int ControlPointCount = Mesh->GetControlPointsCount();
	FbxVector4* ControlPoints = Mesh->GetControlPoints();

	TArray<FVector> ControlPointPositions;
	ControlPointPositions.Reserve(ControlPointCount);

	for (int i = 0; i < ControlPointCount; ++i)
	{
		FVector Pos(ControlPoints[i][0], ControlPoints[i][1], ControlPoints[i][2]);
		if (Config.bConvertToUEBasis)
		{
			Pos = FVector(Pos.X, -Pos.Y, Pos.Z);
		}
		ControlPointPositions.Add(Pos);
	}

	// Material Mapping
	FbxLayerElementMaterial* MaterialElement = Mesh->GetElementMaterial();
	FbxGeometryElement::EMappingMode MaterialMappingMode = FbxGeometryElement::eNone;
	if (MaterialElement)
	{
		MaterialMappingMode = MaterialElement->GetMappingMode();
	}

	// Material별 인덱스 그룹
	TArray<TArray<uint32>> IndicesPerMaterial;
	IndicesPerMaterial.Reset(OutMeshInfo->Materials.Num() > 0 ? OutMeshInfo->Materials.Num() : 1);
	for (int i = 0; i < (OutMeshInfo->Materials.Num() > 0 ? OutMeshInfo->Materials.Num() : 1); ++i)
	{
		IndicesPerMaterial.Add(TArray<uint32>());
	}

	uint32 VertexCounter = 0;

	// 폴리곤별 처리
	const int PolygonCount = Mesh->GetPolygonCount();
	for (int p = 0; p < PolygonCount; ++p)
	{
		int MaterialIndex = 0;
		if (MaterialElement)
		{
			switch (MaterialMappingMode)
			{
			case FbxGeometryElement::eByPolygon:
				MaterialIndex = MaterialElement->GetIndexArray().GetAt(p);
				break;
			case FbxGeometryElement::eAllSame:
				MaterialIndex = 0;
				break;
			}
		}

		if (MaterialIndex < 0 || MaterialIndex >= IndicesPerMaterial.Num())
		{
			MaterialIndex = 0;
		}

		int PolySize = Mesh->GetPolygonSize(p);
		for (int v = 0; v < PolySize; ++v)
		{
			int CtrlPointIndex = Mesh->GetPolygonVertex(p, v);

			// Position
			if (CtrlPointIndex >= 0 && CtrlPointIndex < ControlPointPositions.Num())
			{
				OutMeshInfo->VertexList.Add(ControlPointPositions[CtrlPointIndex]);
			}
			else
			{
				OutMeshInfo->VertexList.Add(FVector(0, 0, 0));
			}

			// Normal
			FbxVector4 Normal;
			if (Mesh->GetPolygonVertexNormal(p, v, Normal))
			{
				FVector N(Normal[0], Normal[1], Normal[2]);
				if (Config.bConvertToUEBasis)
				{
					N = FVector(N.X, -N.Y, N.Z);
				}
				OutMeshInfo->NormalList.Add(N);
			}
			else
			{
				OutMeshInfo->NormalList.Add(FVector(0, 1, 0));
			}

			// UV
			FbxStringList UVSetNames;
			Mesh->GetUVSetNames(UVSetNames);
			if (UVSetNames.GetCount() > 0)
			{
				FbxVector2 UV;
				bool bUnmapped = false;
				if (Mesh->GetPolygonVertexUV(p, v, UVSetNames[0], UV, bUnmapped))
				{
					OutMeshInfo->TexCoordList.Add(FVector2(UV[0], 1.0f - UV[1]));
				}
				else
				{
					OutMeshInfo->TexCoordList.Add(FVector2(0, 0));
				}
			}
			else
			{
				OutMeshInfo->TexCoordList.Add(FVector2(0, 0));
			}

			OutMeshInfo->Indices.Add(VertexCounter);
			IndicesPerMaterial[MaterialIndex].Add(VertexCounter);
			VertexCounter++;
		}
	}

	BuildSkeletalMeshSections(IndicesPerMaterial, OutMeshInfo);
}

void FFbxImporter::ExtractSkeletalMaterials(FbxNode* Node, const std::filesystem::path& FbxFilePath, FFbxSkeletalMeshInfo* OutMeshInfo)
{
	const int MaterialCount = Node->GetMaterialCount();
	UE_LOG("[FbxImporter] Material Count: %d", MaterialCount);

	for (int m = 0; m < MaterialCount; ++m)
	{
		FbxSurfaceMaterial* Material = Node->GetMaterial(m);
		if (!Material) continue;

		FFbxMaterialInfo MatInfo;
		const char* MaterialName = Material->GetName();
		MatInfo.MaterialName = (MaterialName && strlen(MaterialName) > 0)
			? MaterialName
			: "Material_" + std::to_string(m);

		// Diffuse 텍스처 추출
		if (FbxProperty Prop = Material->FindProperty(FbxSurfaceMaterial::sDiffuse); Prop.IsValid())
		{
			int TextureCount = Prop.GetSrcObjectCount<FbxFileTexture>();
			if (TextureCount > 0)
			{
				if (FbxFileTexture* Texture = Prop.GetSrcObject<FbxFileTexture>(0))
				{
					std::string OriginalTexturePath = Texture->GetFileName();
					std::filesystem::path FbxDirectory = FbxFilePath.parent_path();
					std::filesystem::path ResolvedPath = ResolveTexturePath(OriginalTexturePath, FbxDirectory, FbxFilePath);

					if (!ResolvedPath.empty())
					{
						MatInfo.DiffuseTexturePath = ResolvedPath;
					}
				}
			}
		}

		OutMeshInfo->Materials.Add(MatInfo);
	}

	if (OutMeshInfo->Materials.Num() == 0)
	{
		FFbxMaterialInfo DefaultMat;
		DefaultMat.MaterialName = "Default";
		OutMeshInfo->Materials.Add(DefaultMat);
	}
}

void FFbxImporter::BuildSkeletalMeshSections(const TArray<TArray<uint32>>& IndicesPerMaterial, FFbxSkeletalMeshInfo* OutMeshInfo)
{
	uint32 CurrentIndexOffset = 0;
	for (int i = 0; i < IndicesPerMaterial.Num(); ++i)
	{
		if (IndicesPerMaterial[i].Num() > 0)
		{
			FFbxMeshSection Section;
			Section.StartIndex = CurrentIndexOffset;
			Section.IndexCount = IndicesPerMaterial[i].Num();
			Section.MaterialIndex = i;
			OutMeshInfo->Sections.Add(Section);

			CurrentIndexOffset += Section.IndexCount;
		}
	}
}
