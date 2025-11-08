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

bool FFbxImporter::LoadFBX(const std::filesystem::path& FilePath, FFbxMeshInfo* OutMeshInfo, Configuration Config)
{
	// 입력 검증
	if (!OutMeshInfo)
	{
		UE_LOG_ERROR("유효하지 않은 FBXMeshInfo입니다.");
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

void FFbxImporter::ExtractVertices(FbxMesh* Mesh, FFbxMeshInfo* OutMeshInfo, const Configuration& Config)
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

void FFbxImporter::ExtractMaterials(FbxNode* Node, const std::filesystem::path& FbxFilePath, FFbxMeshInfo* OutMeshInfo)
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
	FFbxMeshInfo* OutMeshInfo,
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

	// Material별 인덱스 그룹 초기화
	TArray<TArray<uint32>> IndicesPerMaterial;
	IndicesPerMaterial.Reset(OutMeshInfo->Materials.Num());
	for (int i = 0; i < OutMeshInfo->Materials.Num(); ++i)
	{
		IndicesPerMaterial.Add(TArray<uint32>());
	}

	// 폴리곤별로 인덱스, 노멀, UV 추출
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
			OutMeshInfo->Indices.Add(CtrlPointIndex);
			IndicesPerMaterial[MaterialIndex].Add(CtrlPointIndex);

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
			}
		}
	}

	// Mesh Section 정보 생성
	BuildMeshSections(IndicesPerMaterial, OutMeshInfo);
}

void FFbxImporter::BuildMeshSections(const TArray<TArray<uint32>>& IndicesPerMaterial, FFbxMeshInfo* OutMeshInfo)
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
