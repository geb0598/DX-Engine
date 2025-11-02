#pragma once
#include "Actor/Public/Actor.h"

class AController;

/**
 * @brief Pawn은 플레이어나 AI가 제어할 수 있는 모든 Actor의 베이스 클래스
 */
UCLASS()
class APawn :
	public AActor
{
	DECLARE_CLASS(APawn, AActor)

public:
	APawn();
	virtual ~APawn() override;

	// Possess 함수
	virtual void PossessedBy(AController* NewController);
	virtual void UnPossessed();

	AController* GetController() const { return Controller; }

	bool bUseControllerRotationPitch = false;
	bool bUseControllerRotationYaw = false;
	bool bUseControllerRotationRoll = false;

	float BaseEyeHeight = 64.0f;

protected:
	AController* Controller = nullptr;
};
