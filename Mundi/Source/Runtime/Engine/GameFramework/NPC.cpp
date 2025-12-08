#include "pch.h"
#include "NPC.h"

#include "AudioComponent.h"
#include "CapsuleComponent.h" // 혹은 BoxComponent.h
#include "SkeletalMeshComponent.h"
#include "ResourceManager.h"
#include "Vehicle.h"
#include "FAudioDevice.h"

ANPC::ANPC()
{
    CapsuleComponent = CreateDefaultSubobject<UCapsuleComponent>("");
    CapsuleComponent->ObjectName = "Capsule";
    CapsuleComponent->SetCapsuleSize(0.4f, 0.9f); 
    CapsuleComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    CapsuleComponent->SetSimulatePhysics(false); // NPC는 스스로 물리로 안 움직임 (키네마틱 or 고정)
    CapsuleComponent->OnComponentHit.AddDynamic(this, &ANPC::OnHit);
    SetRootComponent(CapsuleComponent);

    MeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>("");
    MeshComponent->ObjectName = "Mesh";
    MeshComponent->SetupAttachment(CapsuleComponent);
    MeshComponent->SetRelativeLocation(FVector(0, 0, -0.9f)); // 캡슐 바닥으로 내림
    MeshComponent->SetSkeletalMesh(GDataDir + "/Model/Pete.fbx");
    MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision); // 평소엔 메쉬 충돌 끔
    MeshComponent->SetSimulatePhysics(false);
    UPhysicsAsset* DefaultPhysicsAsset = UResourceManager::GetInstance().Load<UPhysicsAsset>("Data/Physics/123.physicsasset");
    if (DefaultPhysicsAsset)
    {
        MeshComponent->SetPhysicsAsset(DefaultPhysicsAsset);
    }

    HitSoundComponent = CreateDefaultSubobject<UAudioComponent>("HitSound");
    HitSoundComponent->SetupAttachment(MeshComponent);
    HitSoundComponent->bAutoPlay = false;
    HitSoundComponent->bIsLooping = false;
    
    DefaultAnimPath = GDataDir + "/SillyDancing_mixamo.com";
    HitSoundPath = GDataDir + "/Audio/Male-hurt-sound-effect.wav";

    // @todo 아직 원인은 파악 못했지만 BeginPlay()에서 SetSound() 호출할 시 크래시 발생, 임시로 이곳에서 초기화함
    if (!HitSoundPath.empty())
    {
        // @todo 임시로 하드코딩된 경로 사용 (씬에 직렬화된 NPC 있어서 현재 크래시 발생함)
        USound* HitSound = UResourceManager::GetInstance().Load<USound>(GDataDir + "/Audio/Male-hurt-sound-effect.wav");
        if (HitSound)
        {
            if (HitSoundComponent)
            {
                HitSoundComponent->SetSound(HitSound);
            }
        }
    }
}

ANPC::~ANPC()
{
}

void ANPC::BeginPlay()
{
    Super::BeginPlay();

    if (!DefaultAnimPath.empty())
    {
        IdleAnimation = UResourceManager::GetInstance().Get<UAnimSequence>(DefaultAnimPath);
        if (IdleAnimation && MeshComponent)
        {
            MeshComponent->PlayAnimation(IdleAnimation, true); // Loop
        }
    }
}

void ANPC::DuplicateSubObjects()
{
    Super::DuplicateSubObjects();

    for (UActorComponent* Component : OwnedComponents)
    {
        if (Component->ObjectName == "Capsule")
        {
            CapsuleComponent = Cast<UCapsuleComponent>(Component);
            if (CapsuleComponent)
            {
                CapsuleComponent->OnComponentHit.Clear(); 
                CapsuleComponent->OnComponentHit.AddDynamic(this, &ANPC::OnHit);
            }
        }
        else if (Component->ObjectName == "Mesh")
        {
            MeshComponent = Cast<USkeletalMeshComponent>(Component);
        }
        else if (UAudioComponent* NewAudioComponent = Cast<UAudioComponent>(Component))
        {
            HitSoundComponent = NewAudioComponent;
        }
    }
}

void ANPC::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
    if (bIsRagdoll) return;

    if (OtherActor && OtherActor->IsA<AVehicle>())
    {
        FVector ImpactDir = GetActorLocation() - OtherActor->GetActorLocation();
        ImpactDir.Normalize();
        ImpactDir += FVector(0, 0, 0.2f);
        ImpactDir.Normalize();

        BecomeRagdoll(ImpactDir);
    }
}

void ANPC::BecomeRagdoll(const FVector& ImpactVelocity)
{
    if (bIsRagdoll) return;
    bIsRagdoll = true;

    CapsuleComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    if (MeshComponent)
    {
        MeshComponent->StopAnimation(); 
        
        MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
        // UPhysicsAsset* NewAsset = UResourceManager::GetInstance().Load<UPhysicsAsset>("Data/Physics/123.physicsasset");
        // MeshComponent->SetPhysicsAsset(NewAsset);
        MeshComponent->SetPhysicsMode(EPhysicsMode::Ragdoll); 
        MeshComponent->SetSimulatePhysics(true);

        FVector Impulse = ImpactVelocity * HitImpulseMultiplier;
        Impulse.Z += 1.0f; 
        
        MeshComponent->AddImpulse(Impulse); 
    }

    if (HitSoundComponent)
    {
        float RandomPitch = 0.9f + FMath::RandRange(0.0f, 0.2f);
        HitSoundComponent->Pitch = RandomPitch;
        HitSoundComponent->Play();
    }
}

void ANPC::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
    Super::Serialize(bInIsLoading, InOutHandle);

    if (bInIsLoading)
    {
        // Super Serialize에서 디폴트 컴포넌트 다 Destroy하는데 멤버 변수로 참조하고 있어서 StandAlone에서 댕글링.
        // 역직렬화 따로 해주고 혹시 모를 상황에 대비해서 nullptr설정
        MeshComponent = nullptr;
        CapsuleComponent = nullptr;

        for (UActorComponent* Component : SceneComponents)
        {
            if (USkeletalMeshComponent* NewMeshComponent = Cast<USkeletalMeshComponent>(Component))
            {
                MeshComponent = NewMeshComponent;
            }
            else if (UCapsuleComponent* NewCapsuleComponent = Cast<UCapsuleComponent>(Component))
            {
                CapsuleComponent = NewCapsuleComponent;
            }
            else if (UAudioComponent* NewAudioComponent = Cast<UAudioComponent>(Component))
            {
                HitSoundComponent = NewAudioComponent;
            }
        }
    }
}