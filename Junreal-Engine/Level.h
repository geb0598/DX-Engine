#pragma once
#include "pch.h"
#include "Object.h"
#include "Actor.h"

class UExponentialHeightFogComponent;
class UBillboardComponent;
class UDecalComponent;
class UPrimitiveComponent;
class UFireBallComponent;
class UFXAAComponent;
class UAmbientLightComponent;
class UDirectionalLightComponent;
class UPointLightComponent;
class USpotLightComponent;


class ULevel : public UObject
{
public:
	DECLARE_CLASS(ULevel, UObject)
	ULevel();
	~ULevel() override;

	void AddActor(AActor* InActor);
	void RemoveActor(AActor* InActor);
	void CollectComponentsToRender();

	template<typename T>
	TArray<T*>& GetComponentList();
	
	template<typename T>
	T* GetComponent();

	const TArray<AActor*>& GetActors() const;
	TArray<AActor*>& GetActors();
private:
	TArray<AActor*> Actors;
	TArray<UExponentialHeightFogComponent*> FogComponentList;
	TArray<UBillboardComponent*> BillboardComponentList;
	TArray<UDecalComponent*> DecalComponentList;
	TArray<UPrimitiveComponent*> PrimitiveComponentList;
	TArray<UFireBallComponent*> FireBallComponentList;
	TArray<UFXAAComponent*> FXAAComponentList;
	UAmbientLightComponent* AmbientLightComponent;
	UDirectionalLightComponent* DirectionalLightComponent;
	TArray<UPointLightComponent*> PointLightComponentList;
	TArray<USpotLightComponent*> SpotLightComponentList;
};

template<> TArray<UExponentialHeightFogComponent*>& ULevel::GetComponentList<UExponentialHeightFogComponent>();
template<> TArray<UBillboardComponent*>& ULevel::GetComponentList<UBillboardComponent>();
template<> TArray<UDecalComponent*>& ULevel::GetComponentList<UDecalComponent>();
template<> TArray<UPrimitiveComponent*>& ULevel::GetComponentList<UPrimitiveComponent>();
template<> TArray<UFireBallComponent*>& ULevel::GetComponentList<UFireBallComponent>();
template<> TArray<UFXAAComponent*>& ULevel::GetComponentList<UFXAAComponent>();
template<> TArray<UPointLightComponent*>& ULevel::GetComponentList<UPointLightComponent>();
template<> TArray<USpotLightComponent*>& ULevel::GetComponentList<USpotLightComponent>();

template<> UAmbientLightComponent* ULevel::GetComponent<UAmbientLightComponent>();
template<> UDirectionalLightComponent* ULevel::GetComponent<UDirectionalLightComponent>();
