#pragma once
#include "Component/LightComponent.h"
class UAmbientLightComponent : public ULightComponent
{
public:
	DECLARE_CLASS(UAmbientLightComponent, ULightComponent)

	UAmbientLightComponent();
	~UAmbientLightComponent() = default;

	UObject* Duplicate() override;

protected:
	void DuplicateSubObjects() override;
	void CopyCommonProperties(UObject* InTarget) override;

private:

};