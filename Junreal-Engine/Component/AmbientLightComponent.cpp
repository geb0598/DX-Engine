#include "pch.h"
#include "Component/AmbientLightComponent.h"

UAmbientLightComponent::UAmbientLightComponent()
{
    SetIntensity(0.2f);
}

UObject* UAmbientLightComponent::Duplicate()
{
    UAmbientLightComponent* DuplicatedComponent = Cast<UAmbientLightComponent>(NewObject(GetClass()));
    if (DuplicatedComponent)
    {
        CopyCommonProperties(DuplicatedComponent);

        // 자식 컴포넌트 복제
        DuplicatedComponent->DuplicateSubObjects();
    }
    return DuplicatedComponent;
}

void UAmbientLightComponent::DuplicateSubObjects()
{
    Super_t::DuplicateSubObjects();
}

void UAmbientLightComponent::CopyCommonProperties(UObject* InTarget)
{
    Super_t::CopyCommonProperties(InTarget);
}
