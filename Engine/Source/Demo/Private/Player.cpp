#include "pch.h"
#include "Demo/Public/Player.h"
#include "Component/Public/ScriptComponent.h"

IMPLEMENT_CLASS(APlayer, AActor)
APlayer::APlayer()
{
	bCanEverTick = true;
	ScriptComponent = CreateDefaultSubobject<UScriptComponent>();
	ScriptComponent->SetScriptName(FName("Player"));
}
