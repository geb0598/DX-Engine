#include "pch.h"
#include "GameVictoryVolume.h"
#include "PrimitiveComponent.h"
#include "HudExampleGameMode.h"
#include "GameModeBase.h"

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
}

void AGameVictoryVolume::EndPlay()
{
}

void AGameVictoryVolume::OnBeginOverlap(UPrimitiveComponent* MyComp, UPrimitiveComponent* OtherComp)
{
}

void AGameVictoryVolume::DuplicateSubObjects()
{
	Super::DuplicateSubObjects();

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
	}
}
