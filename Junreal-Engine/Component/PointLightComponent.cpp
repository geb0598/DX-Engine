#include "pch.h"
#include "PointLightComponent.h"
#include "Resource/DebugDrawManager.h"

#include "BillboardComponent.h"

#include "pch.h"
#include "PointLightComponent.h"
#include "Resource/DebugDrawManager.h"
#include "BillboardComponent.h"

UPointLightComponent::UPointLightComponent()
	: AttenuationRadius(15.0f), LightFalloffExponent(1.0f)
{
    SetTickEnabled(true);
}

UPointLightComponent::~UPointLightComponent()
{
}

const float UPointLightComponent::GetAttenuationRadius() const
{
	return AttenuationRadius;
}

void UPointLightComponent::SetAttenuationRadius(float InRadius)
{
	AttenuationRadius = InRadius;
}

const float UPointLightComponent::GetLightFalloffExponent() const
{
	return LightFalloffExponent;
}

void UPointLightComponent::SetLightFalloffExponent(float InLightFalloffExponent)
{
	LightFalloffExponent = InLightFalloffExponent;
}

void UPointLightComponent::TickComponent(float DeltaTime)
{
	Super_t::TickComponent(DeltaTime);

	if (!GetWorld() || GetWorld()->IsPIEWorld())
	{
		return;
	}

	FDebugDrawManager& DebugDrawer = FDebugDrawManager::GetInstance();
	const FVector Center = GetWorldLocation();
	const FColor Color = GetLightColor();
	const FVector4 DrawColor(Color.R / 255.f, Color.G / 255.f, Color.B / 255.f, 1.f);

	DebugDrawer.AddSphere(Center, AttenuationRadius, 24, DrawColor);
}

UObject* UPointLightComponent::Duplicate()
{
    UPointLightComponent* DuplicatedComponent = Cast<UPointLightComponent>(NewObject(GetClass()));
    if (DuplicatedComponent)
    {
        CopyCommonProperties(DuplicatedComponent);

        // 자식 컴포넌트 복제
        DuplicatedComponent->DuplicateSubObjects();
    }
    return DuplicatedComponent;
}

void UPointLightComponent::DuplicateSubObjects()
{
    Super_t::DuplicateSubObjects();
}

void UPointLightComponent::CopyCommonProperties(UObject* InTarget)
{
    // 부모 속성 복제
    Super_t::CopyCommonProperties(InTarget);

    // 자신의 속성 복제
    UPointLightComponent* Target = Cast<UPointLightComponent>(InTarget);
    if (Target)
    {
        Target->LightFalloffExponent = LightFalloffExponent;
		Target->AttenuationRadius = AttenuationRadius;
    }
}