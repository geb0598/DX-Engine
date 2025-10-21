#include "pch.h"
#include "BillboardComponent.h"
#include "ResourceManager.h"
#include "VertexData.h"
#include "CameraActor.h"
#include "SceneLoader.h"

UBillboardComponent::UBillboardComponent()
{
    SetRelativeLocation({ 0, 0, 0 });

    auto& ResourceManager = UResourceManager::GetInstance();

    // 빌보드용 메시 가져오기 (단일 쿼드)
    BillboardQuad = ResourceManager.Get<UTextQuad>("Billboard");

    // 머티리얼 생성 또는 가져오기
   /* if (auto* M = ResourceManager.Get<UMaterial>("Billboard"))
    {
        Material = M;
        SetMaterial("Billboard.hlsl");
    }
    else
    {
        Material = NewObject<UMaterial>();
        ResourceManager.Add<UMaterial>("Billboard", Material);
    }*/
    SetMaterial("Billboard.hlsl");//메테리얼 자동 매칭
}

UBillboardComponent::~UBillboardComponent()
{
}

void UBillboardComponent::SetTexture(const FString& InTexturePath)
{
    TexturePath = InTexturePath;
}

void UBillboardComponent::SetTintColor(const FColor& InColor)
{
    TintColor = InColor;
}

void UBillboardComponent::SetUVCoords(float U, float V, float UL, float VL)
{
    UCoord = U;
    VCoord = V;
    ULength = UL;
    VLength = VL;
}

UObject* UBillboardComponent::Duplicate()
{
  
    UBillboardComponent* DuplicatedComponent = Cast<UBillboardComponent>(NewObject(GetClass()));
    if (DuplicatedComponent)
    {
        CopyCommonProperties(DuplicatedComponent);
		DuplicatedComponent->TexturePath = TexturePath;
        DuplicatedComponent->SetEditable(this->bEdiableWhenInherited);
        DuplicatedComponent->SetBillboardSize(this->BillboardSize);
        DuplicatedComponent->SetTintColor(TintColor);
        DuplicatedComponent->DuplicateSubObjects();
        
    }
    return DuplicatedComponent;
}

void UBillboardComponent::DuplicateSubObjects()
{
    Super_t::DuplicateSubObjects();
}

void UBillboardComponent::Serialize(bool bIsLoading, FComponentData& InOut)
{
    // 0) 트랜스폼 직렬화/역직렬화는 상위(UPrimitiveComponent)에서 처리
    UPrimitiveComponent::Serialize(bIsLoading, InOut);

    if (bIsLoading)
    {
        // TexturePath 로드
        if (!InOut.TexturePath.empty())
        {
            SetTexture(InOut.TexturePath);
        }
    }
    else
    {
        // TexturePath 저장
        InOut.TexturePath = TexturePath;
    }
}

void UBillboardComponent::CreateBillboardVertices()
{
    TArray<FBillboardVertexInfo_GPU> vertices;

    // 단일 쿼드의 4개 정점 생성 (카메라를 향하는 평면)
    // 중심이 (0,0,0)이고 크기가 BillboardWidth x BillboardHeight인 사각형
    float half = BillboardSize * 0.5f;

    FBillboardVertexInfo_GPU Info;

    // 정점 0: 좌상단 (-halfW, +halfH)
    Info.Position[0] = -half;
    Info.Position[1] = half;
    Info.Position[2] = 0.0f;
    Info.CharSize[0] = BillboardSize;
    Info.CharSize[1] = BillboardSize;
    Info.UVRect[0] = UCoord;   // u start
    Info.UVRect[1] = VCoord;   // v start
    Info.UVRect[2] = ULength;  // u length (UL)
    Info.UVRect[3] = VLength;  // v length (VL)
    vertices.push_back(Info);

    // 정점 1: 우상단 (+halfW, +halfH)
    Info.Position[0] = half;
    Info.Position[1] = half;
    Info.Position[2] = 0.0f;
    vertices.push_back(Info);

    // 정점 2: 좌하단 (-halfW, -halfH)
    Info.Position[0] = -half;
    Info.Position[1] = -half;
    Info.Position[2] = 0.0f;
    vertices.push_back(Info);

    // 정점 3: 우하단 (+halfW, -halfH)
    Info.Position[0] = half;
    Info.Position[1] = -half;
    Info.Position[2] = 0.0f;
    vertices.push_back(Info);

    // 동적 버텍스 버퍼 업데이트
    UResourceManager::GetInstance().UpdateDynamicVertexBuffer("Billboard", vertices);
}

void UBillboardComponent::Render(URenderer* Renderer, const FMatrix& View, const FMatrix& Proj, const EEngineShowFlags ShowFlags)
{
    if (HasShowFlag(ShowFlags, EEngineShowFlags::SF_BillboardText) == false)
    {
        return;
    }
    // 텍스처 로드
    Material->Load(TexturePath, Renderer->GetRHIDevice()->GetDevice());

    // 카메라 정보 가져오기
    ACameraActor* CameraActor = GetOwner()->GetWorld()->GetCameraActor();
    FVector CamRight = CameraActor->GetActorRight();
    FVector CamUp = CameraActor->GetActorUp();

    // 빌보드 위치 설정
    FVector BillboardPos = GetWorldLocation();
    if (this->IsEditable())
    {
        FVector BillboardScale = GetRelativeScale();
        float Scale = BillboardScale.X > BillboardScale.Y ? BillboardScale.X : BillboardScale.Y;
        SetBillboardSize(Scale);
    }
    // 상수 버퍼 업데이트
    ////UUID만 필요하지만 기존 버퍼와 함수 재사용하기 위해서 모델버퍼 받아옴
    Renderer->UpdateSetCBuffer(ModelBufferType(FMatrix(), this->InternalIndex));

    FVector4 NormalizedColor(TintColor.R / 255.f, TintColor.G / 255.f, TintColor.B / 255.f, TintColor.A / 255.f);
    BillboardBufferType BillboardCB;
    BillboardCB.pos = BillboardPos;
    BillboardCB.padding = 0;
    BillboardCB.InverseViewMat = View.InverseAffine();
    BillboardCB.TintColor = NormalizedColor; // 빌보드 스프라이트의 컬러를 넘겨줌
    Renderer->UpdateSetCBuffer(BillboardCB);

    Renderer->OMSetDepthStencilState(EComparisonFunc::Disable);
    
    Renderer->OMSetBlendState(false);
    // 셰이더 준비
    Renderer->PrepareShader(Material->GetShader());

    // 빌보드 정점 생성 및 버퍼 업데이트
    CreateBillboardVertices();

    // 렌더링
    
    Renderer->RSSetState(EViewModeIndex::VMI_Unlit);
    Renderer->DrawIndexedPrimitiveComponent(this, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}
