#include "RayCaster/Public/URayCaster.h"

const float URayCaster::EPSILON = 1e-5f;

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

	return GetRealWorldDistance(RayCastToSphere());
}

std::optional<float> URayCaster::GetHitResultAtScreenPosition(
	UPlaneComponent& PlaneComponent,
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

	return GetRealWorldDistance(RayCastToQuad());
}
