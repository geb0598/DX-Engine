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

	// 3. 인덱스 및 노멀 / UV 추출
	const int PolygonCount = FoundMesh->GetPolygonCount();
	for (int p = 0; p < PolygonCount; ++p)
	{
		// Triangulate를 거쳤기 때문에 PolygonSize는 항상 3입니다.
		int PolySize = FoundMesh->GetPolygonSize(p);
		for (int v = 0; v < PolySize; ++v)
		{
			int CtrlPointIndex = FoundMesh->GetPolygonVertex(p, v);
			OutMeshInfo->Indices.Add(CtrlPointIndex);

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

	// 4. 재질(Material) 및 텍스처 정보 추출
	if (FoundNode)
	{
		FbxSurfaceMaterial* Material = FoundNode->GetMaterial(0);
		if (Material)
		{
			OutMeshInfo->MaterialName = Material->GetName();

			FbxProperty Prop = Material->FindProperty(FbxSurfaceMaterial::sDiffuse);
			if (Prop.IsValid())
			{
				int LayeredTextureCount = Prop.GetSrcObjectCount<FbxLayeredTexture>();
				if (LayeredTextureCount > 0)
				{
					// TODO: Layered Texture 처리
				}
				else
				{
					int TextureCount = Prop.GetSrcObjectCount<FbxFileTexture>();
					if (TextureCount > 0)
					{
						FbxFileTexture* Texture = Prop.GetSrcObject<FbxFileTexture>(0);
						if (Texture)
						{
							OutMeshInfo->DiffuseTexturePath = Texture->GetFileName();
						}
					}
				}
			}
		}
	}


	Scene->Destroy();
	return true;
}
