#include "pch.h"
#include "NPC.h"
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

    DefaultAnimPath = GDataDir + "/SillyDancing_mixamo.com";
    HitSoundPath = GDataDir + "/Audio/Hit.wav";
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

    if (!HitSoundPath.empty())
    {
        HitSound = UResourceManager::GetInstance().Load<USound>(HitSoundPath);
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

    if (HitSound)
    {
        FAudioDevice::PlaySound3D(HitSound, GetActorLocation());
    }
}