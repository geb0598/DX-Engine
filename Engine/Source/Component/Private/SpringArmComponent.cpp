#include "pch.h"
#include "Component/Public/SpringArmComponent.h"
IMPLEMENT_CLASS(USpringArmComponent, USceneComponent)


USpringArmComponent::USpringArmComponent()
{
	TargetArmLength = 300.0f;
	bEnableCameraLag = false;
	CameraLagSpeed = 10.0f;

	PreviousLocation = FVector::ZeroVector();
	bIsFirstUpdate = true;
	bCanEverTick = true;
}

void USpringArmComponent::BeginPlay()
{
	Super::BeginPlay();

	// 초기 위치 설정
	PreviousLocation = GetWorldLocation() + FVector(-TargetArmLength, 0.0f, 0.0f);
	bIsFirstUpdate = true;
}

void USpringArmComponent::TickComponent(float DeltaTime)
{
	Super::TickComponent(DeltaTime);
	UpdateCamera(DeltaTime);
	bIsFirstUpdate = false;
}

void USpringArmComponent::UpdateCamera(float DeltaTime)
{
	FVector TargetLocation = GetWorldLocation() - GetForwardVector() * TargetArmLength;

	FVector FinalLocation = TargetLocation;
	if (bEnableCameraLag && !bIsFirstUpdate)
	{
		float Alpha = Clamp(CameraLagSpeed * DeltaTime, 0.0f, 1.0f);
		FinalLocation = FVector::Lerp(PreviousLocation, TargetLocation, Alpha);
	}

	// 3. 자식 컴포넌트 위치 업데이트
	const TArray<USceneComponent*>& Children = GetChildren();
	for (USceneComponent* Child : Children)
	{
		if (Child)
		{
			Child->SetWorldLocation(FinalLocation);
		}
	}

	PreviousLocation = FinalLocation;
}
