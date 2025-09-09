#include <optional>
#include <utility>
#include <memory>

#include <Windows.h>

#include "Containers/Containers.h"
#include "Window/Public/Mouse.h"
#include "Types/Types.h"

void UMouse::Flush()
{
	MouseEventBuffer = std::queue<UEvent>();
}

std::pair<int32, int32> UMouse::GetPosition() const
{
	return { GetXPosition(), GetYPosition() };
}

int UMouse::GetXPosition() const
{
	return MouseState.X;
}

int UMouse::GetYPosition() const
{
	return MouseState.Y;
}

bool UMouse::IsInsideWindow() const
{
	return MouseState.bIsInsideWindow;
}

bool UMouse::IsLeftPressed() const
{
	return MouseState.bIsLeftPressed;
}

bool UMouse::IsRightPressed() const
{
	return MouseState.bIsRightPressed;
}

bool UMouse::IsEmpty() const
{
	return MouseEventBuffer.empty();
}

std::optional<UMouse::UEvent> UMouse::Read()
{
	std::optional<UEvent> Result;
	if (!IsEmpty())
	{
		Result = MouseEventBuffer.front();
		MouseEventBuffer.pop();
	}
	return Result;
}

void UMouse::OnMouseMove(int X, int Y)
{
	MouseState.X = X;
	MouseState.Y = Y;
	UEvent Event{ UEvent::EEventType::MOVE, MouseState };
	MouseEventBuffer.push(Event);
	TrimBuffer();
}

void UMouse::OnMouseLeave()
{
	MouseState.bIsInsideWindow = false;
	UEvent Event{ UEvent::EEventType::LEAVE, MouseState };
	MouseEventBuffer.push(Event);
	TrimBuffer();
}

void UMouse::OnMouseEnter()
{
	MouseState.bIsInsideWindow = true;
	UEvent Event{ UEvent::EEventType::ENTER, MouseState };
	MouseEventBuffer.push(Event);
	TrimBuffer();
}

void UMouse::OnLeftPressed(int X, int Y)
{
	MouseState.bIsLeftPressed = true;
	UEvent Event{ UEvent::EEventType::L_PRESS, MouseState };
	MouseEventBuffer.push(Event);
	TrimBuffer();
}

void UMouse::OnLeftReleased(int X, int Y)
{
	MouseState.bIsLeftPressed = false;
	UEvent Event{ UEvent::EEventType::L_PRESS, MouseState };
	MouseEventBuffer.push(Event);
	TrimBuffer();
}

void UMouse::OnRightPressed(int X, int Y)
{
	MouseState.bIsRightPressed = true;
	UEvent Event{ UEvent::EEventType::R_PRESS, MouseState };
	MouseEventBuffer.push(Event);
	TrimBuffer();
}

void UMouse::OnRightReleased(int X, int Y)
{
	MouseState.bIsRightPressed = false;
	UEvent Event{ UEvent::EEventType::R_PRESS, MouseState };
	MouseEventBuffer.push(Event);
	TrimBuffer();
}

void UMouse::OnWheelUp(int X, int Y)
{
	UEvent Event{ UEvent::EEventType::WHEEL_UP, MouseState };
	MouseEventBuffer.push(Event);
	TrimBuffer();
}

void UMouse::OnWheelDown(int X, int Y)
{
	UEvent Event{ UEvent::EEventType::WHEEL_DOWN, MouseState };
	MouseEventBuffer.push(Event);
	TrimBuffer();
}

void UMouse::TrimBuffer()
{
	while (MouseEventBuffer.size() > BUFFER_SIZE)
	{
		MouseEventBuffer.pop();
	}
}

void UMouse::OnWheelDelta(int X, int Y, int Delta)
{
	MouseState.WheelDeltaCarry += Delta;
	while (MouseState.WheelDeltaCarry >= WHEEL_DELTA)
	{
		MouseState.WheelDeltaCarry -= WHEEL_DELTA;
		OnWheelUp(X, Y);
	}
	while (MouseState.WheelDeltaCarry <= -WHEEL_DELTA)
	{
		MouseState.WheelDeltaCarry += WHEEL_DELTA;
		OnWheelDown(X, Y);
	}
}