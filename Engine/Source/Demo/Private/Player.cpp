#include "pch.h"
#include "Demo/Public/Player.h"

#include "Component/Mesh/Public/StaticMeshComponent.h"
#include "Component/Public/BoxComponent.h"
#include "Component/Public/LightSensorComponent.h"
#include "Component/Public/ScriptComponent.h"
#include "Component/Public/LightSensorComponent.h"

IMPLEMENT_CLASS(APlayer, AActor)
APlayer::APlayer()
{
	bCanEverTick = true;
	CreateDefaultSubobject<UScriptComponent>()->SetScriptName("Player");
	SetCollisionTag(ECollisionTag::Player);
}

UClass* APlayer::GetDefaultRootComponent()
{
	return UBoxComponent::StaticClass();
}

void APlayer::InitializeComponents()
{
	Super::InitializeComponents();
	Cast<UBoxComponent>(GetRootComponent())->SetBoxExtent({0.5f, 0.5f, 0.5f});

	ULightSensorComponent* LightSensor = CreateDefaultSubobject<ULightSensorComponent>();
	LightSensor->AttachToComponent(GetRootComponent());

	UStaticMeshComponent* StaticMesh = CreateDefaultSubobject<UStaticMeshComponent>();
	StaticMesh->AttachToComponent(GetRootComponent());
	StaticMesh->SetStaticMesh("Data/Capsule.obj");
	StaticMesh->SetRelativeLocation(FVector{0, 0, -7});
	StaticMesh->SetRelativeScale3D(FVector{4, 4, 7});
}
