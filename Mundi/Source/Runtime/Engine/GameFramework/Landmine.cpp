#include "pch.h"
#include "Landmine.h"

#include "AudioComponent.h"
#include "ParticleSystemComponent.h"
#include "StaticMeshComponent.h"
#include "PrimitiveComponent.h"

ALandmine::ALandmine()
{
    ObjectName = "Landmine";

    MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>("MeshComponent");
    MeshComponent->SetStaticMesh(GDataDir + "/Model/Landmine/Landmine.obj");
    MeshComponent->SetSimulatePhysics(false); 
    MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    MeshComponent->OnComponentHit.AddDynamic(this, &ALandmine::OnHit);
    RootComponent = MeshComponent;

    FlashingEffect = CreateDefaultSubobject<UParticleSystemComponent>("");
    FlashingEffect->ObjectName = "FlashingEffect";
    FlashingEffect->SetupAttachment(MeshComponent);
    FlashingEffect->SetRelativeLocation(FVector(0, 0, 0.75f));
    FlashingEffect->bAutoActivate = true;

    UParticleSystem* FlashingTemplate = UResourceManager::GetInstance().Load<UParticleSystem>(GDataDir + "/Particles/MineFlash.particle");
    if (FlashingTemplate)
    {
        FlashingEffect->SetTemplate(FlashingTemplate);
    }

    ExplosionEffect = CreateDefaultSubobject<UParticleSystemComponent>("");
    ExplosionEffect->ObjectName = "ExplosionEffect";
    ExplosionEffect->SetupAttachment(RootComponent);
    ExplosionEffect->bAutoActivate = false;

    UParticleSystem* ExplosionTemplate = UResourceManager::GetInstance().Load<UParticleSystem>(GDataDir + "/Particles/explosion.particle");
    if (ExplosionTemplate)
    {
        ExplosionEffect->SetTemplate(ExplosionTemplate);
    }

    ExplosionSoundComponent = CreateDefaultSubobject<UAudioComponent>("");
    ExplosionSoundComponent->SetupAttachment(RootComponent);
    USound* ExplosionSound = UResourceManager::GetInstance().Load<USound>(GDataDir + "/Audio/explosion-fx.wav");
    ExplosionSoundComponent->SetSound(ExplosionSound);
    ExplosionSoundComponent->bIsLooping = false;
    ExplosionSoundComponent->bAutoPlay = false;

    bIsActive = true;
}

ALandmine::~ALandmine()
{
}

void ALandmine::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
    if (!bIsActive)
    {
        return;
    }

    if ((OtherActor != nullptr) && (OtherActor != this) && (OtherComp != nullptr))
    {
        if (OtherComp->bSimulatePhysics)
        {
            FVector ExplosionDir = OtherActor->GetActorLocation() - GetActorLocation();
            ExplosionDir.Normalize();

            ExplosionDir += FVector(0.0f, 0.0f, 1.0f) * UpwardBias;
            ExplosionDir.Normalize();

            FVector TorqueAxis(
                FMath::RandRange(-1.0f, 1.0f),
                FMath::RandRange(-1.0f, 1.0f),
                FMath::RandRange(-1.0f, 1.0f)
            );

            if (FBodyInstance* BodyInst = OtherComp->GetBodyInstance())
            {
                BodyInst->AddImpulse(ExplosionDir * ExplosionImpulse);
                BodyInst->AddAngularImpulse(TorqueAxis * AngularImpulseStrength);
            }

            if (ExplosionSoundComponent)
            {
                float RandomPitch = 0.9f + FMath::RandRange(0.0f, 0.2f);
                ExplosionSoundComponent->Pitch = RandomPitch;
                ExplosionSoundComponent->Play();
            }

            Deactivate();
        }
    }
}

void ALandmine::Activate()
{
    bIsActive = true;

    if (ExplosionEffect)
    {
        ExplosionEffect->DeactivateSystem(); 
    }
}

void ALandmine::Deactivate()
{
    if (!bIsActive) return;

    bIsActive = false;
    UE_LOG("Explosion");

    if (ExplosionEffect)
    {
        UE_LOG("Effect");
        ExplosionEffect->ActivateSystem();
    }
}

void ALandmine::DuplicateSubObjects()
{
    Super::DuplicateSubObjects();

    for (UActorComponent* Component : OwnedComponents)
    {
        if (UStaticMeshComponent* Mesh = Cast<UStaticMeshComponent>(Component))
        {
            MeshComponent = Mesh;
            MeshComponent->OnComponentHit.Clear(); // 중복 방지
            MeshComponent->OnComponentHit.AddDynamic(this, &ALandmine::OnHit);
        }
        else if (UParticleSystemComponent* Particle = Cast<UParticleSystemComponent>(Component))
        {
            if (Particle->ObjectName == "ExplosionEffect")
            {
                ExplosionEffect = Particle;
            }
            else if (Particle->ObjectName == "FlashingEffect")
            {
                FlashingEffect = Particle;
            }
        }
        else if (UAudioComponent* Audio = Cast<UAudioComponent>(Component))
        {
            ExplosionSoundComponent = Audio; 
        }
    }
}

void ALandmine::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
    Super::Serialize(bInIsLoading, InOutHandle);

    if (bInIsLoading)
    {
        MeshComponent = Cast<UStaticMeshComponent>(RootComponent);
        
        if (MeshComponent)
        {
             MeshComponent->OnComponentHit.AddDynamic(this, &ALandmine::OnHit);
        }

        for (UActorComponent* Component : OwnedComponents)
        {
            if (UParticleSystemComponent* Particle = Cast<UParticleSystemComponent>(Component))
            {
                if (Particle->ObjectName == "ExplosionEffect")
                {
                    ExplosionEffect = Particle;
                }
                else if (Particle->ObjectName == "FlashingEffect")
                {
                    FlashingEffect = Particle;
                }
            }
        }
    }
}