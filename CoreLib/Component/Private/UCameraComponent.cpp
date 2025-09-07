#include "../Public/UCameraComponent.h"

const double UCameraComponent::DefaultFOV = 90.0;
const double UCameraComponent::DefaultNearPlane = 0.1;
const double UCameraComponent::DefaultFarPlane = 1000.0;

UCameraComponent::UCameraComponent(AActor* Actor)
	: UActorComponent(Actor),
	FOV(DefaultFOV),
	NearPlane(DefaultNearPlane),
	FarPlane(DefaultFarPlane)
{}

UCameraComponent::UCameraComponent(AActor* Actor,
	double FOVToSet,
	double NPToSet,
	double FPToSet)
	: UActorComponent(Actor),
	FOV(FOVToSet),
	NearPlane(NPToSet),
	FarPlane(FPToSet)
{}

double UCameraComponent::GetFOV() const { return FOV; }
void UCameraComponent::SetFOV(double FOVToSet) { FOV = FOVToSet; }

double UCameraComponent::GetNearPlane() const { return NearPlane; }
void UCameraComponent::SetNearPlane(double NPToSet) { NearPlane = NPToSet; }

double UCameraComponent::GetFarPlane() const { return FarPlane; }
void UCameraComponent::SetFarPlane(double FPToSet) { FarPlane = FPToSet; }

FMatrix UCameraComponent::GetViewMatrix()
{
	// Get the info of scene component from camera's owner, actor
	AActor* Actor = GetActor();
	assert(Actor != nullptr);
	USceneComponent* SC = Actor->GetComponent<USceneComponent>();

	assert(SC != nullptr);

	FVector CamLocation = SC->GetLocation();
	FVector CamRotation = SC->GetRotation();

	return FMatrix::CreateView(CamLocation, CamRotation);
}

FMatrix UCameraComponent::GetProjectionMatrix(float AspectRatio) 
{ 
	return FMatrix::CreatePerspective(DEG_TO_RAD(FOV), AspectRatio, NearPlane, FarPlane); 
}

