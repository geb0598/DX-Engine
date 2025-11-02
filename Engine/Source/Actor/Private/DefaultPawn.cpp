#include "pch.h"
#include "Actor/Public/DefaultPawn.h"

#include "Component/Public/BoxComponent.h"
#include "Component/Public/ScriptComponent.h"

IMPLEMENT_CLASS(ADefaultPawn, APawn)

ADefaultPawn::ADefaultPawn()
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
	Cast<UBoxComponent>(GetRootComponent())->SetBoxExtent({1.0f, 1.0f, 1.0f});
}
