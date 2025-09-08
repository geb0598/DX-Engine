#include "../Public/UCameraComponent.h"

const float UCameraComponent::DefaultFieldOfView = 90.0f;
const float UCameraComponent::DefaultNearPlane = 0.1;
const float UCameraComponent::DefaultFarPlane = 1000.0;

UCameraComponent::UCameraComponent(AActor* Actor)
	: UActorComponent(Actor),
	FieldOfView(DefaultFieldOfView),
	NearPlane(DefaultNearPlane),
	FarPlane(DefaultFarPlane),
	bIsOrthogonal(false)
{}

UCameraComponent::UCameraComponent(AActor* Actor,
	float FieldOfViewToSet,
	float NearPlaneToSet,
	float FarPlaneToSet)
	: UActorComponent(Actor),
	FieldOfView(FieldOfViewToSet),
	NearPlane(NearPlaneToSet),
	FarPlane(FarPlaneToSet)
{}

float UCameraComponent::GetFieldOfView() const 
{ 
	return FieldOfView;
}

void UCameraComponent::SetFieldOfView(float FOVToSet) 
{ 
	FieldOfView = FOVToSet; 
}

float UCameraComponent::GetNearPlane() const 
{
	return NearPlane;
}

void UCameraComponent::SetNearPlane(float NPToSet) 
{ 
	NearPlane = NPToSet;
}

float UCameraComponent::GetFarPlane() const 
{
	return FarPlane; 
}

void UCameraComponent::SetFarPlane(float FPToSet) 
{
	FarPlane = FPToSet; 
}

bool UCameraComponent::IsOrthogonal() const
{
	return bIsOrthogonal;
}

bool UCameraComponent::EnableOrthogonal()
{
	return bIsOrthogonal = true;
}

bool UCameraComponent::DisableOrthogonal()
{
	return bIsOrthogonal = false;
}

FMatrix UCameraComponent::GetViewMatrix()
{
	auto SceneComponent = GetActor()->GetComponent<USceneComponent>();
	auto Rotation = SceneComponent->GetRotation();

	// NOTE: Angle should be sent as radian to these functions
	auto PitchMatrix = FMatrix::CreateRotationX(DEG_TO_RAD(Rotation.X));
	auto YawMatrix = FMatrix::CreateRotationY(DEG_TO_RAD(Rotation.Y));
	auto RollMatrix = FMatrix::CreateRotationZ(DEG_TO_RAD(Rotation.Z));

	auto Eye = SceneComponent->GetLocation();
	//auto At = Eye + (FVector(0.0f, 0.0f, 1.0f) * RollMatrix * PitchMatrix * YawMatrix);
	auto At = Eye + (FVector(0.0f, 0.0f, 1.0f) * RollMatrix * PitchMatrix * YawMatrix);
	auto Up = (FVector(0.0f, 1.0f, 0.0f) * RollMatrix * PitchMatrix * YawMatrix);

	return FMatrix::CreateLookAt(Eye, At, Up);
}

FMatrix UCameraComponent::GetProjectionMatrix(float AspectRatio) 
{ 
	return FMatrix::CreatePerspective(DEG_TO_RAD(FieldOfView), AspectRatio, NearPlane, FarPlane);
}

FMatrix UCameraComponent::GetOrthographicMatrix(float Left, float Right, float Bottom, float Top)
{
	return FMatrix::CreateOrthographic(Left, Right, Bottom, Top, NearPlane, FarPlane);
}

