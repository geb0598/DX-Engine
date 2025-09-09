#pragma once

#include <memory>

#include <d3d11.h>

#include "Component/Public/ActorComponent.h"
#include "Mesh/Mesh.h"
#include "Shader/Shader.h"
#include "Types/Types.h"
#include "Window/Public/Keyboard.h"
#include "Window/Public/Mouse.h"

// TODO: InputComponent should update internal delta time to calculate next transform
class UInputComponent : public UActorComponent
{
public:
	virtual ~UInputComponent() = default;

	UInputComponent(AActor* Actor);

	bool IsKeyboardInputEnabled();
	void EnableKeyboardInput();
	void DisableKeyboardInput();

	bool IsMouseInputEnabled();
	void EnableMouseInput();
	void DisableMouseInput();

	float GetMoveSensitivity() const;
	float GetHorizontalTurnSensitivity() const;
	float GetVerticalTurnSensitivity() const;

	void SetMouse(UMouse* Mouse);
	void SetKeyboard(UKeyboard* Keyboard);

	void SetMoveSensitivity(float NewMoveSensitivity);
	void SetHorizontalTurnSensitivity(float NewHorizontalTurnSensitivity);
	void SetVerticalTurnSensitivity(float NewVerticalTurnSensitivity);

	void Update(float DeltaTimeSeconds);

private:
	static constexpr float MAX_PITCH = 89.0f;
	static constexpr float MIN_PITCH = -89.0f;

	bool bIsKeyboardInputEnabled;
	bool bIsMouseInputEnabled;

	UKeyboard* Keyboard;
	UMouse* Mouse;

	float MoveSensitivity = 5.0f;
	float HorizontalTurnSensitivity = 5.0f;;
	float VerticalTurnSensitivity = 5.0f;

	bool bIsMouseRightButtonPressed = false;
	int32 CapturedMousePositionX = 0;
	int32 CapturedMousePositionY = 0;
	FVector CapturedRotation;
};
