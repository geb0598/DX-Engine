#include "pch.h"
#include "ClothComponent.h"
#include "Source/Runtime/Engine/Cloth/ClothManager.h"
#include "D3D11RHI.h"

UClothComponent::UClothComponent()
{
	nv::cloth::ClothMeshDesc meshDesc;

	for (int y = 0; y <= 10; y++)
	{
		for (int x = 0; x <= 10; x++)
		{
			FVertexDynamic Vert;
			Vert.Position = FVector(x, y, 0);
			Vert.Normal = FVector(0, 0, 1);
			Vert.UV = FVector2D(x / 10.0f, y / 10.0f);
			Vertices.push_back(Vert);
			float InvMass = 1.0f;
			Particles.push_back(physx::PxVec4(Vert.Position.X, Vert.Position.Y, Vert.Position.Z, InvMass));
		}
	}

	for (int y = 0; y < 10; y++)
	{
		for (int x = 0; x < 10; x++)
		{
			int CurIdx = x + y * 11;
			Indices.push_back(CurIdx);
			Indices.push_back(CurIdx + 11);
			Indices.push_back(CurIdx + 12);
			Indices.push_back(CurIdx);
			Indices.push_back(CurIdx + 12);
			Indices.push_back(CurIdx + 1);
		}
	}

	//Fill meshDesc with data
	meshDesc.setToDefault();
	meshDesc.points.data = Vertices.data();
	meshDesc.points.stride = sizeof(FVertexDynamic);
	meshDesc.points.count = Vertices.size();

	meshDesc.triangles.data = Indices.data();
	meshDesc.triangles.stride = sizeof(uint32) * 3;
	meshDesc.triangles.count = Indices.size() / 3;
	//etc. for quads, triangles and invMasses

	physx::PxVec3 gravity(0.0f, -9.8f, 0.0f);
	Fabric = NvClothCookFabricFromMesh(UClothManager::Instance->GetFactory(), meshDesc, gravity, &PhaseTypeInfo);
	Cloth = UClothManager::Instance->GetFactory()->createCloth(GetRange(Particles), *Fabric);

	ID3D11Device* Device = UClothManager::Instance->GetDevice();
	D3D11RHI::CreateVerteBuffer(Device, Vertices, &VertexBuffer);
	D3D11RHI::CreateIndexBuffer(Device, Indices, &IndexBuffer);
}
void UClothComponent::TickComponent(float DeltaTime)
{
	Super::TickComponent(DeltaTime);
}
//
//void UClothComponent::CollectMeshBatches(TArray<FMeshBatchElement>& OutMeshBatchElements, const FSceneView* View)
//{
//	if (!StaticMesh || !StaticMesh->GetStaticMeshAsset())
//	{
//		return;
//	}
//
//	const TArray<FGroupInfo>& MeshGroupInfos = StaticMesh->GetMeshGroupInfo();
//
//	auto DetermineMaterialAndShader = [&](uint32 SectionIndex) -> TPair<UMaterialInterface*, UShader*>
//		{
//			UMaterialInterface* Material = GetMaterial(SectionIndex);
//			UShader* Shader = nullptr;
//
//			if (Material && Material->GetShader())
//			{
//				Shader = Material->GetShader();
//			}
//			else
//			{
//				UE_LOG("UStaticMeshComponent: 머티리얼이 없거나 셰이더가 없어서 기본 머티리얼 사용 section %u.", SectionIndex);
//				Material = UResourceManager::GetInstance().GetDefaultMaterial();
//				if (Material)
//				{
//					Shader = Material->GetShader();
//				}
//				if (!Material || !Shader)
//				{
//					UE_LOG("UStaticMeshComponent: 기본 머티리얼이 없습니다.");
//					return { nullptr, nullptr };
//				}
//			}
//			return { Material, Shader };
//		};
//
//	const bool bHasSections = !MeshGroupInfos.IsEmpty();
//	const uint32 NumSectionsToProcess = bHasSections ? static_cast<uint32>(MeshGroupInfos.size()) : 1;
//
//	for (uint32 SectionIndex = 0; SectionIndex < NumSectionsToProcess; ++SectionIndex)
//	{
//		uint32 IndexCount = 0;
//		uint32 StartIndex = 0;
//
//		if (bHasSections)
//		{
//			const FGroupInfo& Group = MeshGroupInfos[SectionIndex];
//			IndexCount = Group.IndexCount;
//			StartIndex = Group.StartIndex;
//		}
//		else
//		{
//			IndexCount = StaticMesh->GetIndexCount();
//			StartIndex = 0;
//		}
//
//		if (IndexCount == 0)
//		{
//			continue;
//		}
//
//		auto [MaterialToUse, ShaderToUse] = DetermineMaterialAndShader(SectionIndex);
//		if (!MaterialToUse || !ShaderToUse)
//		{
//			continue;
//		}
//
//		FMeshBatchElement BatchElement;
//		// View 모드 전용 매크로와 머티리얼 개인 매크로를 결합한다
//		TArray<FShaderMacro> ShaderMacros = View->ViewShaderMacros;
//		if (0 < MaterialToUse->GetShaderMacros().Num())
//		{
//			ShaderMacros.Append(MaterialToUse->GetShaderMacros());
//		}
//		FShaderVariant* ShaderVariant = ShaderToUse->GetOrCompileShaderVariant(ShaderMacros);
//
//		if (ShaderVariant)
//		{
//			BatchElement.VertexShader = ShaderVariant->VertexShader;
//			BatchElement.PixelShader = ShaderVariant->PixelShader;
//			BatchElement.InputLayout = ShaderVariant->InputLayout;
//		}
//
//		// UMaterialInterface를 UMaterial로 캐스팅해야 할 수 있음. 렌더러가 UMaterial을 기대한다면.
//		// 지금은 Material.h 구조상 UMaterialInterface에 필요한 정보가 다 있음.
//		BatchElement.Material = MaterialToUse;
//		BatchElement.VertexBuffer = StaticMesh->GetVertexBuffer();
//		BatchElement.IndexBuffer = StaticMesh->GetIndexBuffer();
//		BatchElement.VertexStride = StaticMesh->GetVertexStride();
//		BatchElement.IndexCount = IndexCount;
//		BatchElement.StartIndex = StartIndex;
//		BatchElement.BaseVertexIndex = 0;
//		BatchElement.WorldMatrix = GetWorldMatrix();
//		BatchElement.ObjectID = InternalIndex;
//		BatchElement.PrimitiveTopology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
//
//		OutMeshBatchElements.Add(BatchElement);
//	}
//}
UClothComponent::~UClothComponent()
{
	VertexBuffer->Release();
	IndexBuffer->Release();

	Solver* Solver = UClothManager::Instance->GetSolver();
	Fabric->decRefCount();
	Solver->removeCloth(Cloth);
}