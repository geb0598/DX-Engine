#include "RayCaster/Public/URayCaster.h"
#include "Component/Public/StaticMeshComponent.h" // [추가]
#include "Mesh/Mesh.h" // [추가]

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
// [추가] UStaticMeshComponent 충돌 검사 함수 구현
std::optional<float> URayCaster::GetHitResultAtScreenPosition(
	UStaticMeshComponent& StaticMeshComponent,
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
	return GetRealWorldDistance(RayCastToMesh(StaticMeshComponent.GetMesh()));
}