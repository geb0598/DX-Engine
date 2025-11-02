#include "pch.h"
#include "Actor/Public/GameMode.h"

IMPLEMENT_CLASS(AGameMode, AActor)

void AGameMode::BeginPlay()
{
	Super::BeginPlay();
	InitGame();
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
