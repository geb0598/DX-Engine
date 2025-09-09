#include "RayCaster/Public/URayCaster.h"

std::optional<float> URayCaster::GetHitResultAtScreenPosition(
	UTriangleComponent& TriangleComponent, 
	int32 MouseX, 
	int32 MouseY,
	int32 ScreenWidth,
	int32 ScreenHeight,
	const FMatrix& ModelingMatrix, 
	const FMatrix& ViewMatrix, 
	const FMatrix& ProjectionMatrix
)
{
	SetRayWithMouseAndMVP(MouseX, MouseY, ScreenWidth, ScreenHeight, ModelingMatrix, ViewMatrix, ProjectionMatrix);

	return GetRealWorldDistance(RayCastToTriangle());
}

std::optional<float> URayCaster::GetHitResultAtScreenPosition(
	UCubeComponent& CubeComponent, 
	int32 MouseX, 
	int32 MouseY,
	int32 ScreenWidth,
	int32 ScreenHeight,
	const FMatrix& ModelingMatrix, 
	const FMatrix& ViewMatrix, 
	const FMatrix& ProjectionMatrix
)
{
	SetRayWithMouseAndMVP(MouseX, MouseY, ScreenWidth, ScreenHeight, ModelingMatrix, ViewMatrix, ProjectionMatrix);

	return GetRealWorldDistance(RayCastToCube());
}

std::optional<float> URayCaster::GetHitResultAtScreenPosition(
	USphereComponent& SphereComponent,
	int32 MouseX,
	int32 MouseY,
	int32 ScreenWidth,
	int32 ScreenHeight,
	const FMatrix& ModelingMatrix,
	const FMatrix& ViewMatrix,
	const FMatrix& ProjectionMatrix
)
{
	SetRayWithMouseAndMVP(MouseX, MouseY, ScreenWidth, ScreenHeight, ModelingMatrix, ViewMatrix, ProjectionMatrix);

	return GetRealWorldDistance(RayCastToSphere(1.0f));
}
