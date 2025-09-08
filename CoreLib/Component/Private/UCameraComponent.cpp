#include "../Public/UCameraComponent.h"

const float UCameraComponent::DefaultFieldOfView = PIDIV2;
const float UCameraComponent::DefaultNearPlane = 0.1;
const float UCameraComponent::DefaultFarPlane = 1000.0;

UCameraComponent::UCameraComponent(AActor* Actor)
	: UActorComponent(Actor),
	FieldOfViewRad(DefaultFieldOfView),
	NearPlane(DefaultNearPlane),
	FarPlane(DefaultFarPlane)
{}

UCameraComponent::UCameraComponent(AActor* Actor,
	float FieldOfViewToSet,
	float NearPlaneToSet,
	float FarPlaneToSet)
	: UActorComponent(Actor),
	FieldOfViewRad(FieldOfViewToSet),
	NearPlane(NearPlaneToSet),
	FarPlane(FarPlaneToSet)
{}

float UCameraComponent::GetFieldOfView() const 
{ 
	return FieldOfViewRad;
}

void UCameraComponent::SetFieldOfView(float FOVToSet) 
{ 
	FieldOfViewRad = FOVToSet; 
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

FMatrix UCameraComponent::GetViewMatrix()
{
	AActor* Actor = GetActor();
	assert(Actor != nullptr);

	USceneComponent* SceneComponent = Actor->GetComponent<USceneComponent>();
	assert(SceneComponent != nullptr);

	FVector Location = SceneComponent->GetLocation();
	FVector Rotation = SceneComponent->GetRotation();

	return FMatrix::CreateViewMatrix(Location, Rotation);
}

FMatrix UCameraComponent::GetProjectionMatrix(float AspectRatio) 
{ 
	return FMatrix::CreatePerspective(FieldOfViewRad, AspectRatio, NearPlane, FarPlane);
}

FMatrix UCameraComponent::GetOrthographicMatrix(float Left, float Right, float Bottom, float Top)
{
	return FMatrix::CreateOrthographic(Left, Right, Bottom, Top, NearPlane, FarPlane);
}

