#include "pch.h"
#include "Actor/Public/GameMode.h"
#include "Actor/Public/PlayerStart.h"
#include "Actor/Public/PlayerCameraManager.h"
#include "Actor/Public/CameraActor.h"
#include "Component/Public/CameraComponent.h"
#include "Level/Public/Level.h"

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
		AActor* ViewTargetToUse = nullptr;

		// Priority 1: Find first active CameraComponent in level
		TArray<AActor*> AllActors = World->GetLevel()->GetLevelActors();
		for (AActor* Actor : AllActors)
		{
			if (Actor)
			{
				UCameraComponent* CameraComp = Actor->GetComponentByClass<UCameraComponent>();
				if (CameraComp && CameraComp->IsActive())
				{
					ViewTargetToUse = Actor;
					UE_LOG_INFO("GameMode: Using Actor '%s' with active CameraComponent as ViewTarget",
						Actor->GetName().ToString().c_str());
					break;
				}
			}
		}

		// Priority 2: Create default CameraActor at origin
		if (!ViewTargetToUse)
		{
			ACameraActor* DefaultCamera = Cast<ACameraActor>(World->SpawnActor(ACameraActor::StaticClass()));
			if (DefaultCamera)
			{
				DefaultCamera->SetActorLocation(FVector(0, 0, 0));
				DefaultCamera->SetActorRotation(FQuaternion::Identity());
				ViewTargetToUse = DefaultCamera;
				UE_LOG_INFO("GameMode: Created default CameraActor at origin as ViewTarget");
			}
		}

		if (ViewTargetToUse)
		{
			PlayerCameraManager->SetViewTarget(ViewTargetToUse);
		}
		else
		{
			UE_LOG_ERROR("GameMode: Failed to create or find camera ViewTarget");
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
