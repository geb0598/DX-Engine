#include "RayCaster/Public/URayCaster.h"

std::optional<float> URayCaster::GetHitResultAtScreenPosition(
	UTriangleComponent& TriangleComponent, 
	int32 X, 
	int32 Y, 
	const FMatrix& ModelingMatrix, 
	const FMatrix& ViewMatrix, 
	const FMatrix& ProjectionMatrix
)
{
	SetRayWithMouseAndMVP(X, Y, ModelingMatrix, ViewMatrix, ProjectionMatrix);

	return RayCastToTriangle();
}

std::optional<float> URayCaster::GetHitResultAtScreenPosition(
	UCubeComponent& CubeComponent, 
	int32 X, 
	int32 Y, 
	const FMatrix& ModelingMatrix, 
	const FMatrix& ViewMatrix, 
	const FMatrix& ProjectionMatrix
)
{
	SetRayWithMouseAndMVP(X, Y, ModelingMatrix, ViewMatrix, ProjectionMatrix);

	return RayCastToCube();
}

std::optional<float> URayCaster::GetHitResultAtScreenPosition(USphereComponent& SphereComponent, int32 X, int32 Y, const FMatrix& ModelingMatrix, const FMatrix& ViewMatrix, const FMatrix& ProjectionMatrix)
{
	return RayCastToSphere(1.0f);
}
