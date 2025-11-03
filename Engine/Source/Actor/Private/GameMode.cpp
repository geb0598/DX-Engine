#include "pch.h"
#include "Actor/Public/GameMode.h"
#include "Actor/Public/PlayerStart.h"
#include "Actor/Public/Pawn.h"
#include "Actor/Public/PlayerController.h"
#include "Editor/Public/Camera.h"
#include "Editor/Public/Editor.h"
#include "Manager/LocalPlayer/Public/LocalPlayerManager.h"

IMPLEMENT_CLASS(AGameMode, AActor)

void AGameMode::BeginPlay()
{
	Super::BeginPlay();

	const FWorldSettings& WorldSettings = GWorld->GetWorldSettings();
	if (WorldSettings.DefaultPlayerClass)
	{
		TArray<APlayerStart*> PlayerStarts = GWorld->FindActorsOfClass<APlayerStart>();

		// PlayerController 생성
		PlayerController = GWorld->SpawnActor<APlayerController>();

		// Pawn 생성 (PlayerStart 위치에 스폰)
		if (PlayerStarts.Num() > 0)
		{
			PlayerPawn = Cast<APawn>(GWorld->SpawnActor(
				WorldSettings.DefaultPlayerClass,
				PlayerStarts[0]->GetActorLocation(),
				PlayerStarts[0]->GetActorRotation()
			));
		}
		else
		{
			PlayerPawn = Cast<APawn>(GWorld->SpawnActor(WorldSettings.DefaultPlayerClass));
		}

		// PlayerController가 Pawn을 Possess
		if (PlayerController && PlayerPawn)
		{
			PlayerController->Possess(PlayerPawn);
		}

		// LocalPlayerManager에 Pawn 등록
		ULocalPlayerManager::GetInstance().PossessActor(PlayerPawn);

		MainCamera = GEditor->GetMainCamera();
		if (MainCamera)
		{
			MainCamera->SetViewTarget(PlayerPawn);
		}
	}

	InitGame();
}

void AGameMode::EndPlay()
{
	// LocalPlayerManager에서 먼저 Unpossess
	ULocalPlayerManager::GetInstance().UnpossessActor();

	// PlayerController가 Pawn을 Unpossess (Pawn 삭제 전에)
	if (PlayerController && PlayerPawn)
	{
		PlayerController->UnPossess();
	}

	// 카메라 ViewTarget 해제
	if (MainCamera)
	{
		MainCamera->SetViewTarget(nullptr);
		MainCamera = nullptr;
	}

	// 참조 해제
	PlayerController = nullptr;
	PlayerPawn = nullptr;

	Super::EndPlay();
}

void AGameMode::InitGame()
{
	bGameRunning = false;
	bGameEnded = false;
	OnGameInited.Broadcast();
}

void AGameMode::StartGame()
{
	bGameRunning = true;
	bGameEnded = false;
	OnGameStarted.Broadcast();
}

void AGameMode::EndGame()
{
	bGameRunning = false;
	bGameEnded = true;
	OnGameEnded.Broadcast();
}
