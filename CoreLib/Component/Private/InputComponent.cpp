#include "Component/Public/InputComponent.h"
#include "Component/Public/USceneComponent.h"
#include "Component/Public/UCameraComponent.h"

UInputComponent::UInputComponent(AActor* Actor)
	: 
	UActorComponent(Actor), 
	Keyboard(nullptr), 
	Mouse(nullptr), 
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

void UInputComponent::SetMouse(UMouse* Mouse)
{
	this->Mouse = Mouse;
}

void UInputComponent::SetKeyboard(UKeyboard* Keyboard)
{
	this->Keyboard = Keyboard;
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

	if (!Keyboard || !Mouse)
	{
		return;
	}

	auto SceneComponent = GetActor()->GetComponent<USceneComponent>();
	if (bIsKeyboardInputEnabled)
	{
		FVector DeltaPosition = {};
		if (Keyboard->IsKeyPressed('W'))
		{
			DeltaPosition.Z += 1.0f;
		}
		if (Keyboard->IsKeyPressed('A'))
		{
			DeltaPosition.X -= 1.0f;
		}
		if (Keyboard->IsKeyPressed('S'))
		{
			DeltaPosition.Z -= 1.0f;
		}
		if (Keyboard->IsKeyPressed('D'))
		{
			DeltaPosition.X += 1.0f;
		}
		DeltaPosition.Normalize();
		DeltaPosition *= MoveSensitivity * DeltaTimeSeconds;

		auto CameraComponent = GetActor()->GetComponent<UCameraComponent>();
		FMatrix ViewMatrix = CameraComponent->GetViewMatrix();
		FMatrix InverseViewMatrix = ViewMatrix.Inverse();

		auto WorldDeltaPosition = DeltaPosition * InverseViewMatrix;

		if (Keyboard->IsKeyPressed('E'))
		{
			WorldDeltaPosition.Y += 1.0f * MoveSensitivity * DeltaTimeSeconds;
		}
		if (Keyboard->IsKeyPressed('Q'))
		{
			WorldDeltaPosition.Y -= 1.0f * MoveSensitivity * DeltaTimeSeconds;
		}

		//SceneComponent->TranslateTransform({ 1.0f, 0.0f, 0.0f });
		SceneComponent->TranslateTransform(WorldDeltaPosition);
	}

	if (bIsMouseInputEnabled)
	{
		if (Mouse->IsRightPressed())
		{
			if (!bIsMouseRightButtonPressed)
			{
				CapturedMousePositionX = Mouse->GetXPosition();
				CapturedMousePositionY = Mouse->GetYPosition();
				bIsMouseRightButtonPressed = true;
			}
			else
			{
				int32 DeltaMouseXPosition = static_cast<int32>(Mouse->GetXPosition()) - CapturedMousePositionX;
				int32 DeltaMouseYPosition = static_cast<int32>(Mouse->GetYPosition()) - CapturedMousePositionY;

				CapturedMousePositionX = static_cast<int32>(Mouse->GetXPosition());
				CapturedMousePositionY = static_cast<int32>(Mouse->GetYPosition());

				float DeltaYaw = HorizontalTurnSensitivity * DeltaMouseXPosition * DeltaTimeSeconds;
				float DeltaPitch = VerticalTurnSensitivity * DeltaMouseYPosition * DeltaTimeSeconds;

				auto Rotation = SceneComponent->GetRotation();
				Rotation.Y += DeltaYaw;
				Rotation.X += DeltaPitch;
				Rotation.X = std::clamp(Rotation.X, MIN_PITCH, MAX_PITCH);

				SceneComponent->SetRotation(Rotation);
			}
		}
		else
		{
			bIsMouseRightButtonPressed = false;
		}
	}
}