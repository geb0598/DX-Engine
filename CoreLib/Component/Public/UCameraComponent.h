#pragma once

#include "ActorComponent.h"
#include "USceneComponent.h"
#include "Math/Math.h"

class UCameraComponent : public UActorComponent
{
private:
	double FOV;
	double NearPlane;
	double FarPlane;

	const static double DefaultFOV;
	const static double DefaultNearPlane;
	const static double DefaultFarPlane;

public:
	UCameraComponent(AActor* Actor);
	UCameraComponent(AActor* Actor, double FOVToSet, double NPToSet, double FPToSet);

	double GetFOV() const;
	void SetFOV(double FOVToSet);

	double GetNearPlane() const;
	void SetNearPlane(double NPToSet);

	double GetFarPlane() const;
	void SetFarPlane(double FPToSet);

	FMatrix GetViewMatrix();
	FMatrix GetProjectionMatrix(float AspectRatio);
};
