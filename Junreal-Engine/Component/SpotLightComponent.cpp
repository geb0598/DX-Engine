#include "pch.h"
#include "Component/SpotLightComponent.h"
#include "Resource/DebugDrawManager.h"

USpotLightComponent::USpotLightComponent()
	: InnerConeAngle(10.0f), OuterConeAngle(20.0f)
{
	SetTickEnabled(true);
}

USpotLightComponent::~USpotLightComponent()
{
}

void USpotLightComponent::TickComponent(float DeltaTime)
{
	UActorComponent::TickComponent(DeltaTime);
	if (!GetWorld() || GetWorld()->IsPIEWorld())
	{
		return;
	}

	FDebugDrawManager& DebugDrawer = FDebugDrawManager::GetInstance();
	const FVector Apex = GetWorldLocation();
	const FVector Direction = GetForwardVector();
	const float Height = GetAttenuationRadius();
	const FColor Color = GetLightColor();
	const FVector4 DrawColor(Color.R / 255.f, Color.G / 255.f, Color.B / 255.f, 1.f);
	const FVector4 OuterColor((Color.R / 255.f) * 0.5f, (Color.G / 255.f) * 0.5f, (Color.B / 255.f) * 0.5f, 1.f);

	// Draw Inner Cone
	DebugDrawer.AddCone(Apex, Direction, Height, GetInnerConeAngle(), 16, DrawColor);

	// Draw Outer Cone
	DebugDrawer.AddCone(Apex, Direction, Height, GetOuterConeAngle(), 16, OuterColor);
}

const float USpotLightComponent::GetInnerConeAngle() const
{
	return InnerConeAngle;
}

const float USpotLightComponent::GetOuterConeAngle() const
{
	return OuterConeAngle;
}

void USpotLightComponent::SetInnerConeAngle(float InInnerConeAngle)
{
	InnerConeAngle = InInnerConeAngle;
}

void USpotLightComponent::SetOuterConeAngle(float InOuterConeAngle)
{
	OuterConeAngle = InOuterConeAngle;
}

UObject* USpotLightComponent::Duplicate()
{
	USpotLightComponent* DuplicatedComponent = Cast<USpotLightComponent>(NewObject(GetClass()));
	if (DuplicatedComponent)
	{
		// 공통 속성 복제
		CopyCommonProperties(DuplicatedComponent);
		// 자식 컴포넌트 복제
		DuplicatedComponent->DuplicateSubObjects();
	}
	return DuplicatedComponent;
}

void USpotLightComponent::DuplicateSubObjects()
{
	Super_t::DuplicateSubObjects();
}

void USpotLightComponent::CopyCommonProperties(UObject* InTarget)
{
	Super_t::CopyCommonProperties(InTarget);

	USpotLightComponent* Target = Cast<USpotLightComponent>(InTarget);
	if (Target)
	{
		Target->InnerConeAngle = InnerConeAngle;
		Target->OuterConeAngle = OuterConeAngle;
	}
}