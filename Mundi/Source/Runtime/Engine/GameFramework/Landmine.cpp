#include "pch.h"
#include "Landmine.h"
#include "StaticMeshComponent.h"
#include "PrimitiveComponent.h"

ALandmine::ALandmine()
{
    ObjectName = "Landmine";

    MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>("MeshComponent");
    
    MeshComponent->SetStaticMesh(GDataDir + "/cube-tex.obj");
    
    MeshComponent->SetSimulatePhysics(false); 
    MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    
    MeshComponent->OnComponentHit.AddDynamic(this, &ALandmine::OnHit);

    RootComponent = MeshComponent;

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

            Deactivate();
        }
    }
}

void ALandmine::Activate()
{
    bIsActive = true;
}

void ALandmine::Deactivate()
{
    if (!bIsActive) return;

    bIsActive = false;
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
        
        if (MeshComponent)
        {
             MeshComponent->OnComponentHit.AddDynamic(this, &ALandmine::OnHit);
        }
    }
}