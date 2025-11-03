#pragma once
#include "Actor/Public/Controller.h"

/**
 * @brief 플레이어가 Pawn을 제어하는데 사용하는 Controller
 *
 * PlayerController는 Pawn과 인간 플레이어 사이의 인터페이스 역할
 * 플레이어 입력을 받아서 Pawn에게 전달하고, HUD 및 카메라 제어
 */
UCLASS()
class APlayerController :
	public AController
{
	DECLARE_CLASS(APlayerController, AController)

public:
	APlayerController();
	virtual ~APlayerController() override;

	void BeginPlay() override;
	void Tick(float DeltaSeconds) override;

	bool IsLocalPlayerController() const { return bIsLocalPlayerController; }

protected:
	virtual void ProcessInput(float DeltaTime);
	virtual void UpdateRotation(float DeltaTime);

	bool bIsLocalPlayerController = true; // 로컬 플레이어 여부 (지금 안 씀)

	// Move St
	float MoveSpeed = 30.0f;
	float RotationSpeed = 80.0f;
	FVector CurrentRotation = FVector(0, 0, 0);
};
