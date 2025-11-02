#pragma once
#include "Actor/Public/Actor.h"

class APawn;

/**
 * @brief Controller는 Pawn을 제어하는 비물리적 Actor
 * Possess 메서드로 Pawn을 제어하고, UnPossess로 제어를 해제
 */
UCLASS()
class AController :
	public AActor
{
	DECLARE_CLASS(AController, AActor)

public:
	AController();
	virtual ~AController() override;

	void EndPlay() override;

	// 제어 함수
	virtual void Possess(APawn* InPawn);
	virtual void UnPossess();

	APawn* GetPawn() const { return PossessedPawn; }

	const FQuaternion& GetControlRotation() const { return ControlRotation; }
	void SetControlRotation(const FQuaternion& InRotation) { ControlRotation = InRotation; }

protected:
	APawn* PossessedPawn = nullptr;
	FQuaternion ControlRotation;
};
