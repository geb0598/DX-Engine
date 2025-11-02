#include "pch.h"
#include "Actor/Public/Pawn.h"
#include "Actor/Public/Controller.h"

IMPLEMENT_CLASS(APawn, AActor)

APawn::APawn()
{
	bCanEverTick = true;
	Controller = nullptr;
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;
	BaseEyeHeight = 64.0f;
}

APawn::~APawn()
{
	// 소멸자에서는 안전하게 포인터만 nullptr로 설정
	// (EndPlay에서 이미 UnPossess 호출됨)
	Controller = nullptr;
}

void APawn::PossessedBy(AController* NewController)
{
	if (Controller != NewController)
	{
		AController* OldController = Controller;
		Controller = NewController;

		UE_LOG_INFO("Pawn: PossessedBy Controller (UUID: %llu)", NewController ? NewController->GetUUID() : 0);
	}
}

void APawn::UnPossessed()
{
	if (Controller)
	{
		UE_LOG_INFO("Pawn: UnPossessed from Controller (UUID: %llu)", Controller->GetUUID());
		Controller = nullptr;
	}
}
