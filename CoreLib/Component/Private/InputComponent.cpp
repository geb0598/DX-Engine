#include "Component/Public/InputComponent.h"
#include "Component/Public/USceneComponent.h"
#include "Window/Public/EventPublisher.h"

UInputComponent::UInputComponent(AActor* Actor)
	: UActorComponent(Actor), bIsEnabled(true)
{

}

void UInputComponent::Initiailze(UKeyboard& Keyboard, UMouse& Mouse)
{
	Keyboard.GetEventPublisher().Subscribe(shared_from_this(), &UInputComponent::KeyboardInputDelegate);
	Mouse.GetEventPublisher().Subscribe(shared_from_this(), &UInputComponent::MouseInputDelegate);
}

bool UInputComponent::IsEnabled()
{
	return bIsEnabled;
}

void UInputComponent::Enable()
{
	bIsEnabled = true;
}

void UInputComponent::Disable()
{
	bIsEnabled = false;
}

void UInputComponent::KeyboardInputDelegate(const UKeyboard::UEvent& Event)
{
	if (!bIsEnabled || Event.IsRelease())
	{
		return;
	}

	auto SceneComponent = GetActor()->GetComponent<USceneComponent>();
	// TODO: Use Translate after being implemented
	float Delta = 0.1f;
	switch (Event.GetKeyCode())
	{
	case 'W':
	{
		auto Location = SceneComponent->GetLocation();
		Location.X += Delta;
		SceneComponent->SetLocation(Location);
		break;
	}
	case 'A':
	{
		auto Location = SceneComponent->GetLocation();
		Location.Y -= Delta;
		SceneComponent->SetLocation(Location);
		break;
	}
	case 'S':
	{
		auto Location = SceneComponent->GetLocation();
		Location.X -= Delta;
		SceneComponent->SetLocation(Location);
		break;
	}
	case 'D':
	{
		auto Location = SceneComponent->GetLocation();
		Location.Y += Delta;
		SceneComponent->SetLocation(Location);
		break;
	}
	}
}

void UInputComponent::MouseInputDelegate(const UMouse::UEvent& Event)
{
	if (!bIsEnabled || Event.GetEventType() != UMouse::UEvent::EEventType::MOVE)
	{
		return;
	}

	// TODO: Use Rotate after being implemented
	auto SceneComponent = GetActor()->GetComponent<USceneComponent>();
	auto MousePosition = Event.GetPosition();
	auto Rotation = SceneComponent->GetRotation();
	float Delta = 10.0f;
	Rotation.Y += MousePosition.first - LastMousePosition.first;
	Rotation.Z += MousePosition.second - LastMousePosition.second;
	SceneComponent->SetRotation(Rotation);

	LastMousePosition = MousePosition;
}

