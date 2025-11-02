#include "pch.h"
#include "Demo/Public/DefaultPawn.h"
#include "Component/Public/ScriptComponent.h"
#include "Component/Public/LightSensorComponent.h"

IMPLEMENT_CLASS(ADefaultPawn, APawn)

ADefaultPawn::ADefaultPawn()
{
	bCanEverTick = true;

	ScriptComponent = CreateDefaultSubobject<UScriptComponent>();
	ScriptComponent->SetScriptName(FName("Player"));

	LightSensorComponent = CreateDefaultSubobject<ULightSensorComponent>();
}
