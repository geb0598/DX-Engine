#include "pch.h"
#include "Actor/Public/GameMode.h"
#include "Editor/Public/Camera.h"
#include "Editor/Public/Editor.h"

IMPLEMENT_CLASS(AGameMode, AActor)

void AGameMode::BeginPlay()
{
	Super::BeginPlay();

	const FWorldSettings& WorldSettings = GWorld->GetWorldSettings();
	if (WorldSettings.DefaultPlayerClass)
	{
		Player = GWorld->SpawnActor(WorldSettings.DefaultPlayerClass);
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
