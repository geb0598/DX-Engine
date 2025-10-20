#pragma once
#include "Component/LightComponentBase.h"

class UDirectionalLightComponent : public ULightComponentBase
{
public:
	DECLARE_CLASS(UDirectionalLightComponent, ULightComponentBase)

	UDirectionalLightComponent();
	~UDirectionalLightComponent();

	void TickComponent(float DeltaTime) override;

	UObject* Duplicate() override;

protected:
	void DuplicateSubObjects() override;
	void CopyCommonProperties(UObject* InTarget) override;
};