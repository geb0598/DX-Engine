#include "RayCaster/Public/URayCaster.h"

const float URayCaster::EPSILON = 1e-5f;

std::optional<float> URayCaster::GetHitResultAtScreenPosition(
	UPrimitiveComponent& PrimitiveComponent,
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

	const auto& VertexArray = PrimitiveComponent.GetMesh()->GetVertexArray();
	std::optional<float> OptTParam;
	for (uint32 i = 0; i < VertexArray.size(); i += 3)
	{
		auto OptResult = MollerTrumbore(
			VertexArray[i].Position,
			VertexArray[i + 1].Position,
			VertexArray[i + 2].Position
		);

		if (!OptResult)
		{
			continue;
		}

		if (!OptTParam || OptResult->TParam < OptTParam)
		{
			OptTParam = OptResult->TParam;
		}
	}

	return OptTParam;
}

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
