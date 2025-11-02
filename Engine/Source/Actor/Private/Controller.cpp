#include "pch.h"
#include "Actor/Public/Controller.h"
#include "Actor/Public/Pawn.h"

IMPLEMENT_CLASS(AController, AActor)

AController::AController()
{
	bCanEverTick = true;
	PossessedPawn = nullptr;
	ControlRotation = FQuaternion::Identity();
}

AController::~AController()
{
	// EndPlay에서 이미 UnPossess 호출되었으므로 소멸자에서는 안전하게 포인터만 nullptr로 설정
	PossessedPawn = nullptr;
}

void AController::EndPlay()
{
	// Pawn Unpossess를 먼저 처리
	if (PossessedPawn)
	{
		UnPossess();
	}

	Super::EndPlay();
}

void AController::Possess(APawn* InPawn)
{
	if (!InPawn)
	{
		return;
	}

	// 이미 다른 Pawn을 제어 중이면 먼저 해제
	if (PossessedPawn)
	{
		UnPossess();
	}

	PossessedPawn = InPawn;
	PossessedPawn->PossessedBy(this);

	UE_LOG_INFO("Controller: Possessed Pawn (UUID: %u)", PossessedPawn->GetUUID());
}

void AController::UnPossess()
{
	if (PossessedPawn)
	{
		APawn* OldPawn = PossessedPawn;
		PossessedPawn = nullptr;

		// Level 소멸 시 Pawn이 먼저 삭제될 수 있기 때문에 Pawn이 아직 유효한 경우에만 UnPossessed 호출
		if (OldPawn)
		{
			OldPawn->UnPossessed();
			UE_LOG_INFO("Controller: Unpossessed Pawn (UUID: %u)", OldPawn->GetUUID());
		}
	}
}
