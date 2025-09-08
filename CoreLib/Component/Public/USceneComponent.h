#pragma once

#include "ActorComponent.h"
#include "Math/Math.h"

class USceneComponent : public UActorComponent
{
private:
	FVector Location;
	FVector Rotation;
	FVector Scale;

	const static FVector DefaultLocation;
	const static FVector DefaultRotation;
	const static FVector DefaultScale;
public:
	~USceneComponent() = default;

	USceneComponent(AActor* Actor);

	USceneComponent(AActor* Actor,
		const FVector& Location,
		const FVector& Rotation,
		const FVector& Scale);

	const FVector& GetLocation() const;
	void SetLocation(const FVector& LocationToSet);
	
	const FVector& GetRotation() const;
	void SetRotation(const FVector& RotationToSet);
	
	const FVector& GetScale() const;
	void SetScale(const FVector& ScaleToSet);

	void TranslateTransform(const FVector& T);
	void RotateTranform(const FVector& R);
	void ScaleTransform(const FVector& S);

	FMatrix GetModelingMatrix() const;
};