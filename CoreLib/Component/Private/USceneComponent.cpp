#include "../Public/USceneComponent.h"

/* private */

const FVector USceneComponent::DefaultLocation(0.0f, 0.0f, 0.0f);
const FVector USceneComponent::DefaultRotation(0.0f, 0.0f, 0.0f);
const FVector USceneComponent::DefaultScale(1.0f, 1.0f, 1.0f);


/* public */

USceneComponent::USceneComponent(AActor* Actor)
	: UActorComponent(Actor),
	Location(DefaultLocation),
	Rotation(DefaultRotation),
	Scale(DefaultScale)
{
}

USceneComponent::USceneComponent(AActor* Actor,
	const FVector& Location,
	const FVector& Rotation,
	const FVector& Scale)
	: UActorComponent(Actor),
	Location(Location),
	Rotation(Rotation),
	Scale(Scale)
{
}

const FVector& USceneComponent::GetLocation() const { return Location; }
void USceneComponent::SetLocation(const FVector& LocationToSet) { Location = LocationToSet; }

const FVector& USceneComponent::GetRotation() const { return Rotation; }
void USceneComponent::SetRotation(const FVector& RotationToSet) { Rotation = RotationToSet; }

const FVector& USceneComponent::GetScale() const { return Scale; }
void USceneComponent::SetScale(const FVector& ScaleToSet) { Scale = ScaleToSet; }

void USceneComponent::TranslateTransform(const FVector& T)
{
	Location += T;
}

void USceneComponent::RotateTranform(const FVector& R)
{
	Rotation += R;
}

void USceneComponent::ScaleTransform(const FVector& S)
{
	Scale += S;
}

FMatrix USceneComponent::GetModelingMatrix() const
{
	return FMatrix::CreateModelTransform(Location, Rotation, Scale);
}

