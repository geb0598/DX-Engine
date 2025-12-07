#include "pch.h"
#include "Landmine.h"
#include "StaticMeshComponent.h"
#include "PrimitiveComponent.h"

ALandmine::ALandmine()
{
    ObjectName = "Landmine";

    // 1. 메시 컴포넌트 생성 및 설정
    MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>("MeshComponent");
    
    // 지뢰 모델 설정 (임시로 큐브 사용, 실제 자산이 있다면 변경)
    MeshComponent->SetStaticMesh(GDataDir + "/cube-tex.obj");
    
    // 2. 물리/충돌 설정
    // 지뢰 자체는 물리 시뮬레이션을 꺼서 고정시키거나(Static), 
    // 켜더라도 무거워서 잘 안 밀리게 설정할 수 있습니다. 여기선 고정으로 가정합니다.
    MeshComponent->SetSimulatePhysics(false); 
    MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    
    // 충돌 채널 설정 (예: WorldDynamic 등 상황에 맞게)
    // MeshComponent->CollisionChannel = ECollisionChannel::WorldDynamic;

    // 3. 충돌 이벤트 바인딩
    // PrimitiveComponent.h에 정의된 델리게이트 시그니처에 맞춰 함수를 연결합니다.
    MeshComponent->OnComponentHit.AddDynamic(this, &ALandmine::OnCompHit);

    RootComponent = MeshComponent;
}

ALandmine::~ALandmine()
{
}

void ALandmine::OnCompHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
    // 자기 자신과의 충돌이나, 유효하지 않은 액터는 무시
    if ((OtherActor != nullptr) && (OtherActor != this) && (OtherComp != nullptr))
    {
        // 대상이 물리를 시뮬레이션 중인지 확인
        if (OtherComp->bSimulatePhysics)
        {
            // 1. 날려보낼 방향 계산 (지뢰 위치 -> 대상 위치)
            FVector ExplosionDir = OtherActor->GetActorLocation() - GetActorLocation();
            ExplosionDir.Normalize();

            // 2. 위쪽으로 띄우는 힘 추가 (지뢰 밟으면 보통 위로 솟구치므로)
            ExplosionDir += FVector(0.0f, 0.0f, 1.0f) * UpwardBias;
            ExplosionDir.Normalize();

            FVector TorqueAxis(
                FMath::RandRange(-1.0f, 1.0f),
                FMath::RandRange(-1.0f, 1.0f),
                FMath::RandRange(-1.0f, 1.0f)
            );

            // 3. 충격량 적용
            // 주의: 엔진의 PrimitiveComponent나 BodyInstance에 AddImpulse가 구현되어 있어야 합니다.
            // 제공된 헤더에는 AddImpulse가 명시적으로 안 보이지만, 물리 엔진 연동부(BodyInstance)에 있을 것으로 가정합니다.
            
            // Case A: 컴포넌트에 래퍼 함수가 있는 경우
            // OtherComp->AddImpulse(ExplosionDir * ExplosionImpulse);

            // Case B: BodyInstance를 직접 이용해야 하는 경우 (제공된 헤더 기반 추정)
            if (FBodyInstance* BodyInst = OtherComp->GetBodyInstance())
            {
                 // 엔진 내부 구현에 따라 함수명은 다를 수 있음 (예: AddImpulse, ApplyLinearImpulse 등)
                 // BodyInst->AddImpulse(ExplosionDir * ExplosionImpulse);
                BodyInst->AddImpulse(ExplosionDir * ExplosionImpulse);
                BodyInst->AddAngularImpulse(TorqueAxis * AngularImpulseStrength);
                 
                 // 만약 구현이 아직 없다면 PxRigidBody에 직접 접근해야 할 수도 있습니다.
                 // 예시: BodyInst->GetPxRigidBody()->addForce(...)
            }

            // 로그 출력
            UE_LOG("Landmine Activated! Boomed");

            // 4. (선택사항) 지뢰 제거
            // 폭발 후 지뢰를 사라지게 하려면:
            // Destroy(); 
        }
    }
}

void ALandmine::DuplicateSubObjects()
{
    Super::DuplicateSubObjects();

    // 복제 시 컴포넌트 포인터 재연결
    for (UActorComponent* Component : OwnedComponents)
    {
        if (UStaticMeshComponent* Mesh = Cast<UStaticMeshComponent>(Component))
        {
            MeshComponent = Mesh;
            // 델리게이트는 복제 과정에서 끊길 수 있으므로 다시 바인딩하거나,
            // 엔진의 Duplicate 메커니즘이 델리게이트 복사를 지원하는지 확인해야 합니다.
            // 안전하게 다시 바인딩:
            MeshComponent->OnComponentHit.Clear(); // 중복 방지
            MeshComponent->OnComponentHit.AddDynamic(this, &ALandmine::OnCompHit);
            break;
        }
    }
}

void ALandmine::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
    Super::Serialize(bInIsLoading, InOutHandle);

    if (bInIsLoading)
    {
        MeshComponent = Cast<UStaticMeshComponent>(RootComponent);
        
        // 로드 시에도 델리게이트 재바인딩이 필요할 수 있습니다.
        if (MeshComponent)
        {
             MeshComponent->OnComponentHit.AddDynamic(this, &ALandmine::OnCompHit);
        }
    }
}