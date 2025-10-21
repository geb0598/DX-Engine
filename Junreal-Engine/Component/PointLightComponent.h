#pragma once
#include "Component/LightComponentBase.h"

class UBillboardComponent;

class UPointLightComponent : public ULightComponentBase
{
public:
	DECLARE_CLASS(UPointLightComponent, ULightComponentBase)

	UPointLightComponent();
	~UPointLightComponent();

	const float GetAttenuationRadius() const;
	void SetAttenuationRadius(float InRadius);

	const float GetLightFalloffExponent() const;
	void SetLightFalloffExponent(float InLightFalloffExponent);

	void TickComponent(float DeltaTime) override;

	UObject* Duplicate() override;

protected:
	void DuplicateSubObjects() override;
	void CopyCommonProperties(UObject* InTarget) override;

	UBillboardComponent* IconBillboardComponent = nullptr;

private:
	float AttenuationRadius;
	float LightFalloffExponent;
};