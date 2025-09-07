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
	if (!bIsEnabled || 
		!(Event.GetKeyCode() == 'W' || 
		Event.GetKeyCode() == 'A' ||
		Event.GetKeyCode() == 'S' ||
		Event.GetKeyCode() == 'D')
	)
	{
		return;
	}

	if (Event.IsRelease())
	{
		switch (Event.GetKeyCode())
		{
		case 'W':
			bIsWPressed = false;
			break;
		case 'A':
			bIsAPressed = false;
			break;
		case 'S':
			bIsSPressed = false;
			break;
		case 'D':
			bIsDPressed = false;
			break;
		}
	}

	auto SceneComponent = GetActor()->GetComponent<USceneComponent>();
	// TODO: Use Translate after being implemented
	if (Event.IsPress())
	{
		switch (Event.GetKeyCode())
		{
		case 'W':
		{
			bIsWPressed = true;
			break;
		}
		case 'A':
		{
			bIsAPressed = true;
			break;
		}
		case 'S':
		{
			bIsSPressed = true;
			break;
		}
		case 'D':
		{
			bIsDPressed = true;
			break;
		}
		}
	}

	float Delta = 0.1f;
	auto Location = SceneComponent->GetLocation();
	if (bIsWPressed)
	{
		Location.X += Delta;
	}
	if (bIsAPressed)
	{
		Location.Y -= Delta;
	}
	if (bIsSPressed)
	{
		Location.X -= Delta;
	}
	if (bIsDPressed)
	{
		Location.Y += Delta;
	}
	SceneComponent->SetLocation(Location);
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

