#include "Component/Public/InputComponent.h"
#include "Component/Public/USceneComponent.h"

UInputComponent::UInputComponent(AActor* Actor, UKeyboard& Keyboard, UMouse& Mouse)
	: 
	UActorComponent(Actor), 
	Keyboard(Keyboard), 
	Mouse(Mouse), 
	bIsKeyboardInputEnabled(true),
	bIsMouseInputEnabled(true),
	bIsMouseRightButtonPressed(false)
{

}

bool UInputComponent::IsKeyboardInputEnabled()
{
	return bIsKeyboardInputEnabled;
}

void UInputComponent::EnableKeyboardInput()
{
	bIsKeyboardInputEnabled = true;
}

void UInputComponent::DisableKeyboardInput()
{
	bIsKeyboardInputEnabled = false;
}

bool UInputComponent::IsMouseInputEnabled()
{
	return bIsMouseInputEnabled;
}

void UInputComponent::EnableMouseInput()
{
	bIsMouseInputEnabled = true;
}

void UInputComponent::DisableMouseInput()
{
	bIsMouseInputEnabled = false;
}

float UInputComponent::GetMoveSensitivity() const
{
	return MoveSensitivity;
}

float UInputComponent::GetHorizontalTurnSensitivity() const
{
	return 0.0f;
}

float UInputComponent::GetVerticalTurnSensitivity() const
{
	return 0.0f;
}

void UInputComponent::SetMoveSensitivity(float NewMoveSensitivity)
{
	MoveSensitivity = NewMoveSensitivity;
}

void UInputComponent::SetHorizontalTurnSensitivity(float NewHorizontalTurnSensitivity)
{
	HorizontalTurnSensitivity = NewHorizontalTurnSensitivity;
}

void UInputComponent::SetVerticalTurnSensitivity(float NewVerticalTurnSensitivity)
{
	VerticalTurnSensitivity = NewVerticalTurnSensitivity;
}

void UInputComponent::Update(float DeltaTimeSeconds)
{
	if (!bIsKeyboardInputEnabled && !bIsMouseInputEnabled)
	{
		return;
	}

	auto SceneComponent = GetActor()->GetComponent<USceneComponent>();
	if (bIsKeyboardInputEnabled)
	{
		FVector DeltaPosition = {};
		if (Keyboard.IsKeyPressed('W'))
		{
			DeltaPosition.Z -= 1.0f;
		}
		if (Keyboard.IsKeyPressed('A'))
		{
			DeltaPosition.X += 1.0f;
		}
		if (Keyboard.IsKeyPressed('S'))
		{
			DeltaPosition.Z += 1.0f;
		}
		if (Keyboard.IsKeyPressed('D'))
		{
			DeltaPosition.X -= 1.0f;
		}
		DeltaPosition.Normalize();
		DeltaPosition *= MoveSensitivity * DeltaTimeSeconds;
		auto Rotation = SceneComponent->GetRotation();
		auto WorldDeltaPosition = DeltaPosition * FMatrix::CreateRotationFromEuler(SceneComponent->GetRotation());
		SceneComponent->TranslateTransform(WorldDeltaPosition);
	}

	if (bIsMouseInputEnabled)
	{
		if (Mouse.IsRightPressed())
		{
			if (!bIsMouseRightButtonPressed)
			{
				CapturedMousePositionX = Mouse.GetXPosition();
				CapturedMousePositionY = Mouse.GetYPosition();
				bIsMouseRightButtonPressed = true;
			}
			else
			{
				int32 DeltaMouseXPosition = static_cast<int32>(Mouse.GetXPosition()) - CapturedMousePositionX;
				int32 DeltaMouseYPosition = static_cast<int32>(Mouse.GetYPosition()) - CapturedMousePositionY;

				CapturedMousePositionX = static_cast<int32>(Mouse.GetXPosition());
				CapturedMousePositionY = static_cast<int32>(Mouse.GetYPosition());

				float DeltaYaw = HorizontalTurnSensitivity * DeltaMouseXPosition * DeltaTimeSeconds;
				float DeltaPitch = VerticalTurnSensitivity * DeltaMouseYPosition * DeltaTimeSeconds;

				auto Rotation = SceneComponent->GetRotation();
				Rotation.Y -= DeltaYaw;
				Rotation.X -= DeltaPitch;

				SceneComponent->SetRotation(Rotation);
			}
		}
		else
		{
			bIsMouseRightButtonPressed = false;
		}
	}
}