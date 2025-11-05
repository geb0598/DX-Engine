#include "pch.h"
#include "Actor/Public/GameMode.h"
#include "Actor/Public/PlayerStart.h"
#include "Actor/Public/PlayerCameraManager.h"
#include "Actor/Public/CameraActor.h"

IMPLEMENT_CLASS(AGameMode, AActor)

void AGameMode::BeginPlay()
{
	Super::BeginPlay();

	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG_ERROR("GameMode::BeginPlay: GetWorld() returned nullptr");
		return;
	}

	// TODO: Move to APlayerController::SpawnPlayerCameraManager() when PlayerController is implemented
	// Create PlayerCameraManager
	PlayerCameraManager = Cast<APlayerCameraManager>(World->SpawnActor(APlayerCameraManager::StaticClass()));

	const FWorldSettings& WorldSettings = World->GetWorldSettings();
	if (WorldSettings.DefaultPlayerClass)
	{
		TArray<APlayerStart*> PlayerStarts = World->FindActorsOfClass<APlayerStart>();

		Player = World->SpawnActor(WorldSettings.DefaultPlayerClass);

		if (PlayerStarts.Num() > 0)
		{
			// SpawnActor Location 인자 있는 버전으로 바뀌면 변경
			Player->SetActorLocation(PlayerStarts[0]->GetActorLocation());
			Player->SetActorRotation(PlayerStarts[0]->GetActorRotation());
		}
	}

	// Set ViewTarget for camera system
	if (PlayerCameraManager)
	{
		ACameraActor* CameraToUse = nullptr;

		// Priority 1: Find CameraActor placed in level
		TArray<ACameraActor*> CameraActors = World->FindActorsOfClass<ACameraActor>();
		if (CameraActors.Num() > 0)
		{
			CameraToUse = CameraActors[0];
			UE_LOG_INFO("GameMode: Using existing CameraActor as ViewTarget");
		}
		// Priority 2: Create default CameraActor at origin
		else
		{
			CameraToUse = Cast<ACameraActor>(World->SpawnActor(ACameraActor::StaticClass()));
			if (CameraToUse)
			{
				CameraToUse->SetActorLocation(FVector(0, 0, 0));
				CameraToUse->SetActorRotation(FQuaternion::Identity());
				UE_LOG_INFO("GameMode: Created default CameraActor at origin as ViewTarget");
			}
		}

		if (CameraToUse)
		{
			PlayerCameraManager->SetViewTarget(CameraToUse);
		}
		else
		{
			UE_LOG_ERROR("GameMode: Failed to create or find CameraActor");
		}
	}

	InitGame();
}

void AGameMode::EndPlay()
{
	Super::EndPlay();

	if (PlayerCameraManager)
	{
		PlayerCameraManager->SetViewTarget(nullptr);
	}

	// Player 포인터 정리 (게임 재시작 시 dangling pointer 방지)
	Player = nullptr;
	PlayerCameraManager = nullptr;
}

void AGameMode::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	// PlayerCameraManager updates itself via its own Tick
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
