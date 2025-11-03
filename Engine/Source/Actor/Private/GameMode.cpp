#include "pch.h"
#include "Actor/Public/GameMode.h"
#include "Actor/Public/PlayerStart.h"
#include "Editor/Public/Camera.h"
#include "Editor/Public/Editor.h"

IMPLEMENT_CLASS(AGameMode, AActor)

void AGameMode::BeginPlay()
{
	Super::BeginPlay();

	const FWorldSettings& WorldSettings = GWorld->GetWorldSettings();
	if (WorldSettings.DefaultPlayerClass)
	{
		TArray<APlayerStart*> PlayerStarts = GWorld->FindActorsOfClass<APlayerStart>();

		Player = GWorld->SpawnActor(WorldSettings.DefaultPlayerClass);

		if (PlayerStarts.Num() > 0)
		{
			// SpawnActor Location 인자 있는 버전으로 바뀌면 변경
			Player->SetActorLocation(PlayerStarts[0]->GetActorLocation());
			Player->SetActorRotation(PlayerStarts[0]->GetActorRotation());
		}

		MainCamera = GEditor->GetMainCamera();
		if (MainCamera) { MainCamera->SetViewTarget(Player); }
	}

	InitGame();
}

void AGameMode::EndPlay()
{
	Super::EndPlay();

	if (MainCamera)
	{
		MainCamera->SetViewTarget(nullptr);
	}

	// Player 포인터 정리 (게임 재시작 시 dangling pointer 방지)
	Player = nullptr;
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
