#include "pch.h"
#include "GameVictoryVolume.h"
#include "PrimitiveComponent.h"
#include "HudExampleGameMode.h"
#include "GameModeBase.h"
#include "World.h"
#include "PlayerController.h"
#include "Pawn.h"

AGameVictoryVolume::AGameVictoryVolume()
{
	ObjectName = "Game Victory Volume";

	TriggerVolume = CreateDefaultSubobject<UBoxComponent>("TriggerVolume");
	if (TriggerVolume)
	{
		// 루트로 설정
		RootComponent = TriggerVolume;

		// 트리거 용도 설정
		TriggerVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		TriggerVolume->SetSimulatePhysics(false);
		TriggerVolume->SetGenerateOverlapEvents(true);
		TriggerVolume->bBlockComponent = false;
	}
}

void AGameVictoryVolume::BeginPlay()
{
	Super::BeginPlay();

	if (TriggerVolume)
	{
		TriggerVolume->OnComponentHit.AddDynamic(this, &AGameVictoryVolume::OnHit);
	}
}

void AGameVictoryVolume::EndPlay()
{
	if (TriggerVolume)
	{
		TriggerVolume->OnComponentHit.Clear();
	}

	Super::EndPlay();
}

void AGameVictoryVolume::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (!OtherActor || !OtherComp || bVictoryEventTriggered)
	{
		return;
	}

	// 플레이어 판별: 태그 우선, 없으면 Pawn 캐스팅
	bool bIsPlayer = false;
	if (!PlayerTag.empty())
	{
		bIsPlayer = (OtherActor->GetTag() == PlayerTag);
	}
	if (!bIsPlayer)
	{
		bIsPlayer = OtherActor->IsA(APawn::StaticClass());
	}

	if (!bIsPlayer)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	AGameModeBase* GameMode = World->GetGameMode();
	if (!GameMode)
	{
		return;
	}

	// HUD 예제 게임 모드만 대상으로
	if (AHudExampleGameMode* HudGM = Cast<AHudExampleGameMode>(GameMode))
	{
		UE_LOG("YOU WIN!!! Player has reached the victory volume!!!");
		bVictoryEventTriggered = true;
		HudGM->EndGame(true);
	}
}

void AGameVictoryVolume::DuplicateSubObjects()
{
	Super::DuplicateSubObjects();

	bVictoryEventTriggered = false;

	for (UActorComponent* Component : OwnedComponents)
	{
		if (UBoxComponent* Box = Cast<UBoxComponent>(Component))
		{
			TriggerVolume = Box;
			break;
		}
	}
}

void AGameVictoryVolume::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
	Super::Serialize(bInIsLoading, InOutHandle);

	if (bInIsLoading)
	{
		TriggerVolume = Cast<UBoxComponent>(RootComponent);
		bVictoryEventTriggered = false;
	}
}
