#pragma once
#include "Core/Public/Object.h"

class AActor;

/**
 * @brief 로컬 플레이어 관리 클래스
 * 플레이어가 조종하는 Actor(Pawn)와 입력 상태를 관리
 * Possessed Actor 추적
 * 입력 활성화 / 비활성화 상태 관리
 */
class ULocalPlayerManager
{
public:
	static ULocalPlayerManager& GetInstance();

	// Lifecycle
	void Initialize();
	void Shutdown();

	// Player Control
	void PossessActor(AActor* InActor);
	void UnpossessActor();
	AActor* GetPossessedActor() const { return PossessedActor; }
	bool HasPossessedActor() const { return PossessedActor != nullptr; }

	// Input State
	void SetInputEnabled(bool bEnabled);
	bool IsInputEnabled() const { return bInputEnabled; }

	// Player State
	void SetPlayerActive(bool bActive) { bIsPlayerActive = bActive; }
	bool IsPlayerActive() const { return bIsPlayerActive; }

private:
	ULocalPlayerManager() = default;
	~ULocalPlayerManager() = default;
	ULocalPlayerManager(const ULocalPlayerManager&) = delete;
	ULocalPlayerManager& operator=(const ULocalPlayerManager&) = delete;

	bool bIsInitialized = false;
	bool bIsPlayerActive = false;
	bool bInputEnabled = true;

	// 현재 플레이어가 조종 중인 Actor (Pawn)
	AActor* PossessedActor = nullptr;
};
