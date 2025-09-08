#pragma once

#include "ActorComponent.h"
#include "USceneComponent.h"
#include "Math/Math.h"

class UCameraComponent : public UActorComponent
{
private:
	float FieldOfViewRad;
	float NearPlane;
	float FarPlane;

	const static float DefaultFieldOfView;
	const static float DefaultNearPlane;
	const static float DefaultFarPlane;

public:
	UCameraComponent(AActor* Actor);
	UCameraComponent(AActor* Actor, float FieldOfViewToSet, float NearPlaneToSet, float FarPlaneToSet);

	float GetFieldOfView() const;
	void SetFieldOfView(float FieldOfViewToSet);

	float GetNearPlane() const;
	void SetNearPlane(float NearPlaneToSet);

	float GetFarPlane() const;
	void SetFarPlane(float FarPlaneToSet);

	FMatrix GetViewMatrix();
	FMatrix GetProjectionMatrix(float AspectRatio);
	FMatrix GetOrthographicMatrix(float Left, float Right, float Bottom, float Top);
};
