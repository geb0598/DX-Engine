#include "pch.h"
#include "Component/DirectionalLightComponent.h"
#include "Resource/DebugDrawManager.h"

#include "BillboardComponent.h"

UDirectionalLightComponent::UDirectionalLightComponent()
{
	SetTickEnabled(true);
}

UDirectionalLightComponent::~UDirectionalLightComponent()
{
}

void UDirectionalLightComponent::TickComponent(float DeltaTime)
{
	Super_t::TickComponent(DeltaTime);

	if (!GetWorld() || GetWorld()->IsPIEWorld())
	{
		return;
	}

    FDebugDrawManager& DebugDrawer = FDebugDrawManager::GetInstance();
    const FVector Start = GetWorldLocation();
    const FVector Direction = GetForwardVector();
    const FColor Color = GetLightColor();
    const FVector4 DrawColor(Color.R / 255.f, Color.G / 255.f, Color.B / 255.f, 1.f);

    const float ArrowLength = 8.0f;
    const float HeadLength = 2.0f; 
    const float HeadAngle = 12.0f; 

    const FVector End = Start + Direction * ArrowLength;

    // Main arrow line
    DebugDrawer.AddLine(Start, End, DrawColor);

    // Arrow head
    // AddCone의 첫 인자는 원뿔의 '꼭짓점', 두 번째 인자는 '꼭짓점에서 밑면으로 향하는 방향'입니다.
    // 따라서 화살표 끝(End)이 꼭짓점이 되고, 방향은 -Direction이 맞습니다.
    DebugDrawer.AddCone(End, -Direction, HeadLength, HeadAngle, 12, DrawColor);
}

UObject* UDirectionalLightComponent::Duplicate()
{
    UDirectionalLightComponent* DuplicatedComponent = Cast<UDirectionalLightComponent>(NewObject(GetClass()));
    if (DuplicatedComponent)
    {
        // 공통 속성 복제
        CopyCommonProperties(DuplicatedComponent);
        // 자식 컴포넌트 복제
        DuplicatedComponent->DuplicateSubObjects();
    }
    return DuplicatedComponent;
}

void UDirectionalLightComponent::DuplicateSubObjects()
{
    Super_t::DuplicateSubObjects();
}

void UDirectionalLightComponent::CopyCommonProperties(UObject* InTarget)
{
    Super_t::CopyCommonProperties(InTarget);
}