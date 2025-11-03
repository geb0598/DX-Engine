#include "pch.h"
#include "Actor/Public/PlayerController.h"
#include "Actor/Public/Pawn.h"
#include "Manager/Input/Public/InputManager.h"
#include "Manager/LocalPlayer/Public/LocalPlayerManager.h"

IMPLEMENT_CLASS(APlayerController, AController)

APlayerController::APlayerController()
{
	bCanEverTick = true;
	bIsLocalPlayerController = true;
}

APlayerController::~APlayerController()
{
}

void APlayerController::BeginPlay()
{
	Super::BeginPlay();
	UE_LOG_INFO("PlayerController: BeginPlay");

	// 현재 회전 초기화
	if (PossessedPawn)
	{
		CurrentRotation = PossessedPawn->GetActorRotation().ToEuler();
	}
}

void APlayerController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// 로컬 플레이어 컨트롤러만 입력 처리
	if (bIsLocalPlayerController)
	{
		ProcessInput(DeltaSeconds);
		UpdateRotation(DeltaSeconds);
	}
}

void APlayerController::ProcessInput(float DeltaTime)
{
	APawn* ControlledPawn = GetPawn();
	if (!ControlledPawn)
	{
		return;
	}

	// LocalPlayerManager 입력 차단 체크 (Shift  +F1 Eject)
	if (!ULocalPlayerManager::GetInstance().IsInputEnabled())
	{
		return;
	}

	UInputManager& Input = UInputManager::GetInstance();

	// 이동 입력 처리
	float ForwardScale = 0.0f;
	float RightScale = 0.0f;

	if (Input.IsKeyDown(EKeyInput::W))
	{
		ForwardScale += 1.0f;
	}
	if (Input.IsKeyDown(EKeyInput::S))
	{
		ForwardScale -= 1.0f;
	}
	if (Input.IsKeyDown(EKeyInput::A))
	{
		RightScale -= 1.0f;
	}
	if (Input.IsKeyDown(EKeyInput::D))
	{
		RightScale += 1.0f;
	}

	// 이동 방향 계산
	FVector MoveDirection =
		ControlledPawn->GetActorForwardVector() * ForwardScale + ControlledPawn->GetActorRightVector() * RightScale;

	MoveDirection.Z = 0.0f;

	if (MoveDirection.Length() > 0.0f)
	{
		MoveDirection.Normalize();
		FVector NewLocation = ControlledPawn->GetActorLocation() +
		                      (MoveDirection * MoveSpeed * DeltaTime);
		ControlledPawn->SetActorLocation(NewLocation);
	}
}

void APlayerController::UpdateRotation(float DeltaTime)
{
	APawn* ControlledPawn = GetPawn();
	if (!ControlledPawn)
	{
		return;
	}

	// LocalPlayerManager 입력 차단 체크 (Shift + F1 Eject)
	if (!ULocalPlayerManager::GetInstance().IsInputEnabled())
	{
		return;
	}

	UInputManager& Input = UInputManager::GetInstance();
	FVector MouseDelta = Input.GetMouseDelta();

	// 마우스 입력으로 회전 업데이트
	CurrentRotation.X = 0.0f;  // Roll 고정
	CurrentRotation.Y = CurrentRotation.Y - (MouseDelta.Y * RotationSpeed * DeltaTime);
	CurrentRotation.Z = CurrentRotation.Z + (MouseDelta.X * RotationSpeed * DeltaTime);

	// Pitch 클램핑 (짐벌 락 방지)
	CurrentRotation.Y = Clamp(CurrentRotation.Y, -89.0f, 89.0f);

	// Pawn에 회전 적용
	ControlledPawn->SetActorRotation(FQuaternion::FromEuler(CurrentRotation));
}
