#include "pch.h"
#include "StaticMeshComponent.h"
#include "StaticMesh.h"
#include "Shader.h"
#include "Texture.h"
#include "ResourceManager.h"
#include "ObjManager.h"
#include"CameraActor.h"
#include "SceneLoader.h"

UStaticMeshComponent::UStaticMeshComponent()
{
    SetMaterial("StaticMeshShader.hlsl");
}

UStaticMeshComponent::~UStaticMeshComponent()
{

}

void UStaticMeshComponent::Render(URenderer* Renderer, const FMatrix& ViewMatrix, const FMatrix& ProjectionMatrix, const EEngineShowFlags ShowFlags)
{
    if (HasShowFlag(ShowFlags, EEngineShowFlags::SF_StaticMeshes) == false)
    {
        return;
    }
    if (StaticMesh)
    {
        if (Cast<AGizmoActor>(this->GetOwner()))
        {
            Renderer->OMSetDepthStencilState(EComparisonFunc::Always);
        }
        else
        {
            Renderer->OMSetDepthStencilState(EComparisonFunc::LessEqual);
        }

        Renderer->RSSetNoCullState();

        // Normal transformation을 위한 inverse transpose matrix 계산
        FMatrix WorldMatrix = GetWorldMatrix();
        FMatrix NormalMatrix = WorldMatrix.Inverse().Transpose();

        ModelBufferType ModelBuffer;
        ModelBuffer.Model = WorldMatrix;
        ModelBuffer.UUID = this->InternalIndex;
        ModelBuffer.NormalMatrix = NormalMatrix;

        Renderer->UpdateSetCBuffer(ModelBuffer);
      
        //
        Renderer->PrepareShader(GetMaterial()->GetShader());

        //// PerObject 상수 버퍼(b0) 채우기
        //FPerObjectBufferType PerObjectData;
        //PerObjectData.World = GetWorldMatrix();
        //PerObjectData.View = ViewMatrix;
        //PerObjectData.Projection = ProjectionMatrix;

        //Renderer->UpdateSetCBuffer(PerObjectData);

        Renderer->DrawIndexedPrimitiveComponent(GetStaticMesh(), D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST, MaterailSlots);
    }
}

void UStaticMeshComponent::SetStaticMesh(const FString& PathFileName)
{
	StaticMesh = FObjManager::LoadObjStaticMesh(PathFileName);
    
    const TArray<FGroupInfo>& GroupInfos = StaticMesh->GetMeshGroupInfo();
    if (MaterailSlots.size() < GroupInfos.size())
    {
        MaterailSlots.resize(GroupInfos.size());
    }

    // MaterailSlots.size()가 GroupInfos.size() 보다 클 수 있기 때문에, GroupInfos.size()로 설정
    for (int i = 0; i < GroupInfos.size(); ++i) 
    {
        if (MaterailSlots[i].bChangedByUser == false)
        {
            MaterailSlots[i].MaterialName = GroupInfos[i].InitialMaterialName;
        }
    }
}

void UStaticMeshComponent::Serialize(bool bIsLoading, FPrimitiveData& InOut)
{
    // 0) 트랜스폼 직렬화/역직렬화는 상위(UPrimitiveComponent)에서 처리
    UPrimitiveComponent::Serialize(bIsLoading, InOut);

    if (bIsLoading)
    {
        // 1) 신규 포맷: ObjStaticMeshAsset가 있으면 우선 사용
        if (!InOut.ObjStaticMeshAsset.empty())
        {
            SetStaticMesh(InOut.ObjStaticMeshAsset);
            return;
        }

        // 2) 레거시 호환: Type을 "Data/<Type>.obj"로 매핑
        if (!InOut.Type.empty())
        {
            const FString LegacyPath = "Data/" + InOut.Type + ".obj";
            SetStaticMesh(LegacyPath);
        }
    }
    else
    {
        // 저장 시: 현재 StaticMesh가 있다면 실제 에셋 경로를 기록
        if (UStaticMesh* Mesh = GetStaticMesh())
        {
            InOut.ObjStaticMeshAsset = Mesh->GetAssetPathFileName();
        }
        else
        {
            InOut.ObjStaticMeshAsset.clear();
        }
        // Type은 상위(월드/액터) 정책에 따라 별도 기록 (예: "StaticMeshComp")
    }
}

void UStaticMeshComponent::Serialize(bool bIsLoading, FComponentData& InOut)
{
    // 0) 트랜스폼 직렬화/역직렬화는 상위(UPrimitiveComponent)에서 처리
    UPrimitiveComponent::Serialize(bIsLoading, InOut);

    if (bIsLoading)
    {
        // StaticMesh 로드
        if (!InOut.StaticMesh.empty())
        {
            SetStaticMesh(InOut.StaticMesh);
        }
        // TODO: Materials 로드
    }
    else
    {
        // StaticMesh 저장
        if (UStaticMesh* Mesh = GetStaticMesh())
        {
            InOut.StaticMesh = Mesh->GetAssetPathFileName();
        }
        else
        {
            InOut.StaticMesh.clear();
        }
        // TODO: Materials 저장
    }
}

void UStaticMeshComponent::SetMaterialByUser(const uint32 InMaterialSlotIndex, const FString& InMaterialName)
{
    assert((0 <= InMaterialSlotIndex && InMaterialSlotIndex < MaterailSlots.size()) && "out of range InMaterialSlotIndex");

    if (0 <= InMaterialSlotIndex && InMaterialSlotIndex < MaterailSlots.size())
    {
        MaterailSlots[InMaterialSlotIndex].MaterialName = InMaterialName;
        MaterailSlots[InMaterialSlotIndex].bChangedByUser = true;
    }
    else
    {
        UE_LOG("out of range InMaterialSlotIndex: %d", InMaterialSlotIndex);
    }

    assert(MaterailSlots[InMaterialSlotIndex].bChangedByUser == true);
}
const FAABB UStaticMeshComponent::GetWorldAABB() const
{
    if (StaticMesh == nullptr)
    {
        return FAABB();
    }
    
    //LocalAABB의 8점에 WorldMatrix를 곱해 AABB제작
    const FMatrix& WorldMatrix = GetWorldMatrix();
    const FAABB& LocalAABB = StaticMesh->GetAABB();
    FVector WorldVertex[8];
    for (int i = 0; i < 8; i++)
    {
        WorldVertex[i] = (FVector4(LocalAABB.GetVertex(i), 1) * WorldMatrix).GetVt3();
    }

    return FAABB(WorldVertex, 8);    
}
const FAABB& UStaticMeshComponent::GetLocalAABB() const
{
    if (StaticMesh == nullptr)
    {
        return FAABB();
    }
    return StaticMesh->GetAABB();
}
UObject* UStaticMeshComponent::Duplicate()
{
    UStaticMeshComponent* DuplicatedComponent = Cast<UStaticMeshComponent>(NewObject(GetClass()));

    // 공통 속성 복사 (Transform, AttachChildren) - 부모 헬퍼 사용
    CopyCommonProperties(DuplicatedComponent);

    // StaticMeshComponent 전용 속성 복사
    DuplicatedComponent->Material = this->Material;
    DuplicatedComponent->StaticMesh = this->StaticMesh;
    DuplicatedComponent->MaterailSlots = this->MaterailSlots;

    DuplicatedComponent->DuplicateSubObjects();
    return DuplicatedComponent;
}

void UStaticMeshComponent::DuplicateSubObjects()
{
    // 부모의 깊은 복사 수행 (AttachChildren 재귀 복제)
    Super_t::DuplicateSubObjects();
}
