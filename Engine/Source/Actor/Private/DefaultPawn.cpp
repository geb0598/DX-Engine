#include "pch.h"
#include "Actor/Public/DefaultPawn.h"
#include "Component/Public/ScriptComponent.h"

IMPLEMENT_CLASS(ADefaultPawn, APawn)

ADefaultPawn::ADefaultPawn()
{
	bCanEverTick = true;

	ScriptComponent = CreateDefaultSubobject<UScriptComponent>();
	ScriptComponent->SetScriptName(FName("Player"));
}
