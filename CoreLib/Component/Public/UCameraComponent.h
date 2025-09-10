#pragma once

#include "ActorComponent.h"
#include "USceneComponent.h"
#include "Math/Math.h"

class UCameraComponent : public UActorComponent
{
private:
	float FieldOfView;
	float NearPlane;
	float FarPlane;

	// NOTE: bIsOrthogonal flag does nothing. Caller should check its state and choose which projection matrix to use.
	bool bIsOrthogonal;

	const static float DefaultFieldOfView;
	const static float DefaultNearPlane;
	const static float DefaultFarPlane;

public:
	virtual ~UCameraComponent() = default;

	UCameraComponent(AActor* Actor);
	UCameraComponent(AActor* Actor, float FieldOfViewToSet, float NearPlaneToSet, float FarPlaneToSet);

	float GetFieldOfView() const;
	void SetFieldOfView(float FieldOfViewToSet);

	float GetNearPlane() const;
	void SetNearPlane(float NearPlaneToSet);

	float GetFarPlane() const;
	void SetFarPlane(float FarPlaneToSet);

	bool IsOrthogonal() const;
	bool EnableOrthogonal();
	bool DisableOrthogonal();

	FMatrix GetViewMatrix();
	FMatrix GetProjectionMatrix(float AspectRatio);
	FMatrix GetOrthographicMatrix(float Left, float Right, float Bottom, float Top);
};
