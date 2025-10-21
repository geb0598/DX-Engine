#pragma once
#include "SceneComponent.h"

class UBillboardComponent; // Forward declaration

struct FColor
{
	uint8 R;
	uint8 G;
	uint8 B;
	uint8 A;

	FColor(uint8 InR, uint8 InG, uint8 InB, uint8 InA) :
		R(InR), G(InG), B(InB), A(InA) {}
};

class ULightComponentBase : public USceneComponent
{
public:
	DECLARE_CLASS(ULightComponentBase, USceneComponent)

	ULightComponentBase();
	~ULightComponentBase();

	const float GetIntensity() const;
	const FColor& GetLightColor() const;
	const bool GetVisible() const;

	void SetIntensity(float InIntensity);
	virtual void SetLightColor(const FColor& InLightColor);
	void SetVisible(bool bInVisible);

    // Sets the billboard component used as an icon for this light
    void SetIconBillboardComponent(UBillboardComponent* InIcon);

	UObject* Duplicate() override;

protected:
	void DuplicateSubObjects() override;
	void CopyCommonProperties(UObject* InTarget) override;

	// The billboard component for the editor icon
	UBillboardComponent* IconBillboardComponent = nullptr;

private:
	float Intensity;
	FColor LightColor;
	bool bVisible;
};