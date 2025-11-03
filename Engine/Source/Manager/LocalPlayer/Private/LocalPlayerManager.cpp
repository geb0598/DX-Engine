#include "pch.h"
#include "Manager/LocalPlayer/Public/LocalPlayerManager.h"
#include "Actor/Public/Actor.h"

ULocalPlayerManager& ULocalPlayerManager::GetInstance()
{
	static ULocalPlayerManager Instance;
	return Instance;
}

void ULocalPlayerManager::Initialize()
{
	if (!bIsInitialized)
	{
		bIsPlayerActive = true;
		bInputEnabled = true;
		PossessedActor = nullptr;
		bIsInitialized = true;

		UE_LOG_INFO("LocalPlayerManager: Initialized");
	}
}

void ULocalPlayerManager::Shutdown()
{
	if (bIsInitialized)
	{
		UnpossessActor();
		bIsPlayerActive = false;
		bIsInitialized = false;

		UE_LOG_INFO("LocalPlayerManager: Shutdown");
	}
}

void ULocalPlayerManager::PossessActor(AActor* InActor)
{
	if (PossessedActor == InActor)
	{
		return;
	}

	uint32 PreviousUUID = 0;
	if (PossessedActor)
	{
		PreviousUUID = PossessedActor->GetUUID();
	}

	PossessedActor = InActor;

	uint32 NewUUID = 0;
	if (PossessedActor)
	{
		NewUUID = PossessedActor->GetUUID();
	}

	if (PreviousUUID != 0)
	{
		UE_LOG_INFO("LocalPlayerManager: Unpossessing previous actor (UUID: %u)", PreviousUUID);
	}

	if (NewUUID != 0)
	{
		UE_LOG_INFO("LocalPlayerManager: Possessed actor (UUID: %u)", NewUUID);
	}
}

void ULocalPlayerManager::UnpossessActor()
{
	uint32 UnpossessedUUID = 0;
	if (PossessedActor)
	{
		UnpossessedUUID = PossessedActor->GetUUID();
		PossessedActor = nullptr;
	}

	if (UnpossessedUUID != 0)
	{
		UE_LOG_INFO("LocalPlayerManager: Unpossessed actor (UUID: %u)", UnpossessedUUID);
	}
}

void ULocalPlayerManager::SetInputEnabled(bool bEnabled)
{
	if (bInputEnabled != bEnabled)
	{
		bInputEnabled = bEnabled;
		UE_LOG_INFO("LocalPlayerManager: Input %s", bEnabled ? "enabled" : "disabled");
	}
}
