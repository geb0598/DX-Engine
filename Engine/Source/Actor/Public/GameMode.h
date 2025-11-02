#pragma once
#include "Actor/Public/Actor.h"

DECLARE_DELEGATE(FOnGameInit);
DECLARE_DELEGATE(FOnGameStart);
DECLARE_DELEGATE(FOnGameEnd);

class AGameMode : public AActor
{
	DECLARE_CLASS(AGameMode, AActor)

// Delegate Section
public:
	FOnGameInit OnGameInited;
	FOnGameStart OnGameStarted;
	FOnGameEnd OnGameEnded;

public:
	AGameMode() = default;
	void BeginPlay() override;
	void EndPlay() override;

	virtual void InitGame();
	virtual void StartGame();
	virtual void EndGame();

	bool IsGameRunning() const { return bGameRunning; }
	bool IsGameEnded() const { return bGameEnded; }

protected:
	AActor* Player = nullptr;

private:
	class UCamera* MainCamera = nullptr;
	bool bGameRunning = false;
	bool bGameEnded = false;
};
