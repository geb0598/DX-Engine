#pragma once
#include "Component/Public/SceneComponent.h"

UCLASS()
class USpringArmComponent : public USceneComponent
{
	DECLARE_CLASS(USpringArmComponent, USceneComponent)

public:
	USpringArmComponent();
	virtual ~USpringArmComponent() = default;

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime) override;

	// Settings
	void SetTargetArmLength(float NewLength) { TargetArmLength = NewLength; }
	float GetTargetArmLength() const { return TargetArmLength; }

	void SetEnableCameraLag(bool bEnable) { bEnableCameraLag = bEnable; }
	void SetCameraLagSpeed(float Speed) { CameraLagSpeed = Speed; }

private:
	void UpdateCamera(float DeltaTime);

	// Settings
	float TargetArmLength;      // 기본 300.0f
	bool bEnableCameraLag;      // 기본 false
	float CameraLagSpeed;       // 기본 10.0f

	// Runtime
	FVector PreviousLocation;
	bool bIsFirstUpdate;
	bool bEnableArmLengthLag;
	float ArmLengthLagSpeed;
	FQuaternion LaggedWorldRotation;
	FVector LaggedWorldLocation;
	FVector PreviousChildRelativeLocation;
};
