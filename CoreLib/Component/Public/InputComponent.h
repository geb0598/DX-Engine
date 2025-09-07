#pragma once

#include <memory>

#include <d3d11.h>

#include "Component/Public/ActorComponent.h"
#include "Mesh/Mesh.h"
#include "Shader/Shader.h"
#include "Types/Types.h"
#include "Window/Public/Keyboard.h"
#include "Window/Public/Mouse.h"

class UInputComponent : public UActorComponent, public std::enable_shared_from_this<UInputComponent>
{
public:
	virtual ~UInputComponent() = default;

	UInputComponent(AActor* Actor);

	// This component should be initialized before using
	void Initiailze(UKeyboard& Keyboard, UMouse& Mouse);

	bool IsEnabled();
	void Enable();
	void Disable();

private:
	void KeyboardInputDelegate(const UKeyboard::UEvent& Event);
	void MouseInputDelegate(const UMouse::UEvent& Event);

	bool bIsEnabled;

	std::pair<int32, int32> LastMousePosition;
};
