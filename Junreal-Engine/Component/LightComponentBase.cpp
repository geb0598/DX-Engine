#include "pch.h"
#include "Component/LightComponentBase.h"
#include "BillboardComponent.h"
#include "ObjectFactory.h"

ULightComponentBase::ULightComponentBase()
	: Intensity(1.0f), LightColor({220, 220, 220, 255}), bVisible(true)
{

}

ULightComponentBase::~ULightComponentBase()
{
	if (IconBillboardComponent)
	{
        GetOwner()->DeleteComponent(IconBillboardComponent);
	}       
}

void ULightComponentBase::SetIconBillboardComponent(UBillboardComponent* InIcon)
{
    IconBillboardComponent = InIcon;
    if (IconBillboardComponent)
    {
        IconBillboardComponent->SetEditable(false);
    }
}

const float ULightComponentBase::GetIntensity() const
{
	return Intensity;
}

const FColor& ULightComponentBase::GetLightColor() const
{
	return LightColor;
}

const bool ULightComponentBase::GetVisible() const
{
	return bVisible;
}

void ULightComponentBase::SetIntensity(float InIntensity)
{
	Intensity = InIntensity;
}

void ULightComponentBase::SetLightColor(const FColor& InLightColor)
{
	LightColor = InLightColor;
    if (IconBillboardComponent)
    {
        // if billboard component exists, set sprite color
        IconBillboardComponent->SetTintColor(InLightColor);
    }
}

void ULightComponentBase::SetVisible(bool bInVisible)
{
	bVisible = bInVisible;
}

UObject* ULightComponentBase::Duplicate()
{
    ULightComponentBase* DuplicatedComponent = Cast<ULightComponentBase>(NewObject(GetClass()));
    if (DuplicatedComponent)
    {
        // 공통 속성 복제
        CopyCommonProperties(DuplicatedComponent);
        // 자식 컴포넌트 복제
        DuplicatedComponent->DuplicateSubObjects();
    }
    return DuplicatedComponent;
}

void ULightComponentBase::DuplicateSubObjects()
{
    Super_t::DuplicateSubObjects();
}

void ULightComponentBase::CopyCommonProperties(UObject* InTarget)
{
    Super_t::CopyCommonProperties(InTarget);

    ULightComponentBase* Target = Cast<ULightComponentBase>(InTarget);
    if (Target)
    {
        Target->Intensity = Intensity;
        Target->LightColor = LightColor;
        Target->bVisible = bVisible;
    }
}