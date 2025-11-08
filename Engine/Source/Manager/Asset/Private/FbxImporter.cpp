#include "pch.h"
#include "Source/Manager/Asset/Public/FbxImporter.h"

bool FFbxImporter::Initialize()
{
	if (SdkManager) { return true; }

	// FBX SDK Manager를 생성합니다.
	SdkManager = FbxManager::Create();

	// IoSettings Object를 생성합니다.
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
	if (!OutMeshInfo)
	{
		UE_LOG_ERROR("유효하지 않은 FBXMeshInfo입니다.");
		return false;
	}

	if (!SdkManager || !OutMeshInfo)
	{
		UE_LOG_ERROR("FBX SDK Manager가 존재하지 않습니다.");
		return false;
	}

	if (!std::filesystem::exists(FilePath))
	{
		UE_LOG_ERROR("FBX 파일이 존재하지 않습니다: %s", FilePath.string().c_str());
		return false;
	}

	FbxImporter* Importer = FbxImporter::Create(SdkManager, "");
	if (!Importer->Initialize(FilePath.string().c_str(), -1, IoSettings))
	{
		UE_LOG_ERROR("FBX 파일을 로드 실패했습니다: %s", FilePath.string().c_str());
		Importer->Destroy();
		return false;
	}

	FbxScene* Scene = FbxScene::Create(SdkManager, "Scene");
	Importer->Import(Scene);
	Importer->Destroy();

	// Geometry Converter를 사용하여 모든 지오메트리를 삼각형으로 변환합니다.
	FbxGeometryConverter GeomConverter(SdkManager);
	GeomConverter.Triangulate(Scene, /*replace*/true);

	FbxNode* RootNode = Scene->GetRootNode();
	if (!RootNode)
	{
		UE_LOG_ERROR("FBX 파일을 탐색을 실패했습니다.");
		return false;
	}

	// 1. 첫 번째 Mesh만 로드 (OBJ 구조와 통일)
	FbxMesh* FoundMesh = nullptr;
	FbxNode* FoundNode = nullptr;
	for (int i = 0; i < RootNode->GetChildCount(); ++i)
	{
		FbxNode* Child = RootNode->GetChild(i);
		if (FbxMesh* Mesh = Child->GetMesh())
		{
			FoundMesh = Mesh;
			FoundNode = Child;
			break;
		}
	}

	if (!FoundMesh)
	{
		UE_LOG_ERROR("FBX에 유효한 메시가 없습니다");
		return false;
	}

	// 2. Vertex 추출
	const int ControlPointCount = FoundMesh->GetControlPointsCount();
	FbxVector4* ControlPoints = FoundMesh->GetControlPoints();

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

	// 3. 모든 Material 정보 추출
	const int MaterialCount = FoundNode->GetMaterialCount();
	UE_LOG("[FbxImporter] Material Count: %d", MaterialCount);

	for (int m = 0; m < MaterialCount; ++m)
	{
		FbxSurfaceMaterial* Material = FoundNode->GetMaterial(m);
		if (!Material) continue;

		FFbxMaterialInfo MatInfo;
		const char* MaterialName = Material->GetName();
		if (MaterialName && strlen(MaterialName) > 0)
		{
			MatInfo.MaterialName = MaterialName;
		}
		else
		{
			MatInfo.MaterialName = "Material_" + std::to_string(m);
		}
		UE_LOG("[FbxImporter] Material %d: %s", m, MatInfo.MaterialName.c_str());

		// Diffuse 텍스처 추출
		FbxProperty Prop = Material->FindProperty(FbxSurfaceMaterial::sDiffuse);
		if (Prop.IsValid())
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
					FbxFileTexture* Texture = Prop.GetSrcObject<FbxFileTexture>(0);
					if (Texture)
					{
						std::string OriginalTexturePath = Texture->GetFileName();
						UE_LOG("[FbxImporter] Material %d - Texture Path: %s", m, OriginalTexturePath.c_str());

						// 텍스처 경로 처리
						std::filesystem::path ResolvedTexturePath;
						std::filesystem::path OriginalPath(OriginalTexturePath);
						std::filesystem::path FbxDirectory = FilePath.parent_path();

						// 방법 1: 원본 경로가 유효한지 확인
						if (std::filesystem::exists(OriginalPath))
						{
							ResolvedTexturePath = OriginalPath;
							UE_LOG_SUCCESS("[FbxImporter] 텍스처 찾음 (원본 경로): %s", ResolvedTexturePath.string().c_str());
						}
						// 방법 2: FBX 파일과 같은 디렉토리에서 파일명만으로 찾기
						else
						{
							std::filesystem::path FilenameOnly = OriginalPath.filename();
							std::filesystem::path LocalTexturePath = FbxDirectory / FilenameOnly;

							if (std::filesystem::exists(LocalTexturePath))
							{
								ResolvedTexturePath = LocalTexturePath;
								UE_LOG_SUCCESS("[FbxImporter] 텍스처 찾음 (FBX 디렉토리): %s", ResolvedTexturePath.string().c_str());
							}
							// 방법 3: .fbm 폴더에서 찾기 (FBX SDK 기본 텍스처 저장 위치)
							else
							{
								std::filesystem::path FbxFilename = FilePath.stem();  // 확장자 제외한 파일명
								std::filesystem::path FbmFolder = FbxDirectory / (FbxFilename.string() + ".fbm");
								std::filesystem::path FbmTexturePath = FbmFolder / FilenameOnly;

								if (std::filesystem::exists(FbmTexturePath))
								{
									ResolvedTexturePath = FbmTexturePath;
									UE_LOG_SUCCESS("[FbxImporter] 텍스처 찾음 (.fbm 폴더): %s", ResolvedTexturePath.string().c_str());
								}
								else
								{
									UE_LOG_WARNING("[FbxImporter] 텍스처를 찾을 수 없습니다: %s", OriginalTexturePath.c_str());
									UE_LOG_WARNING("[FbxImporter] 시도한 경로: %s", FbmTexturePath.string().c_str());
								}
							}
						}

						if (!ResolvedTexturePath.empty())
						{
							MatInfo.DiffuseTexturePath = ResolvedTexturePath;
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

	// 4. Material Mapping 정보 가져오기
	FbxLayerElementMaterial* MaterialElement = FoundMesh->GetElementMaterial();
	FbxGeometryElement::EMappingMode MaterialMappingMode = FbxGeometryElement::eNone;
	if (MaterialElement)
	{
		MaterialMappingMode = MaterialElement->GetMappingMode();
		UE_LOG("[FbxImporter] Material Mapping Mode: %d", (int)MaterialMappingMode);
	}

	// 5. 인덱스 및 노멀 / UV 추출 (Material별로 분리)
	struct FTempSection
	{
		TArray<uint32> Indices;
		uint32 MaterialIndex;
	};
	TArray<FTempSection> TempSections;
	TempSections.Reset(OutMeshInfo->Materials.Num());
	for (int i = 0; i < TempSections.Num(); ++i)
	{
		TempSections[i].MaterialIndex = i;
	}

	const int PolygonCount = FoundMesh->GetPolygonCount();
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

		// Material Index가 유효한 범위인지 확인
		if (MaterialIndex < 0 || MaterialIndex >= OutMeshInfo->Materials.Num())
		{
			MaterialIndex = 0;
		}

		// Triangulate를 거쳤기 때문에 PolygonSize는 항상 3입니다.
		int PolySize = FoundMesh->GetPolygonSize(p);
		for (int v = 0; v < PolySize; ++v)
		{
			int CtrlPointIndex = FoundMesh->GetPolygonVertex(p, v);
			OutMeshInfo->Indices.Add(CtrlPointIndex);
			TempSections[MaterialIndex].Indices.Add(CtrlPointIndex);

			// --- Normal ---
			FbxVector4 Normal;
			if (FoundMesh->GetPolygonVertexNormal(p, v, Normal))
			{
				FVector N(Normal[0], Normal[1], Normal[2]);
				if (Config.bConvertToUEBasis)
				{
					N = FVector(N.X, -N.Y, N.Z);
				}
				OutMeshInfo->NormalList.Add(N);
			}

			// --- UV ---
			FbxStringList UVSetNames;
			FoundMesh->GetUVSetNames(UVSetNames);
			if (UVSetNames.GetCount() > 0)
			{
				FbxVector2 UV;
				bool bUnmapped = false;
				if (FoundMesh->GetPolygonVertexUV(p, v, UVSetNames[0], UV, bUnmapped))
				{
					FVector2 UVConv(UV[0], 1.0f - UV[1]);
					OutMeshInfo->TexCoordList.Add(UVConv);
				}
			}
		}
	}

	// 6. Section 정보 생성
	uint32 CurrentIndexOffset = 0;
	for (int i = 0; i < TempSections.Num(); ++i)
	{
		if (TempSections[i].Indices.Num() > 0)
		{
			FFbxMeshSection Section;
			Section.StartIndex = CurrentIndexOffset;
			Section.IndexCount = TempSections[i].Indices.Num();
			Section.MaterialIndex = TempSections[i].MaterialIndex;
			OutMeshInfo->Sections.Add(Section);

			UE_LOG("[FbxImporter] Section %d: StartIndex=%d, Count=%d, MaterialIndex=%d",
				i, Section.StartIndex, Section.IndexCount, Section.MaterialIndex);

			CurrentIndexOffset += Section.IndexCount;
		}
	}


	Scene->Destroy();
	return true;
}
