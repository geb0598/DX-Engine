#include <optional>
#include <utility>

#include <Windows.h>

#include "Types/Types.h"
#include "Containers/Containers.h"
#include "Window/Public/Mouse.h"

void UMouse::Flush()
{
	MouseEventBuffer = std::queue<UEvent>();
}

std::pair<int, int> UMouse::GetPosition() const
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
	MouseEventBuffer.push(UEvent(UEvent::EEventType::MOVE, MouseState));
}

void UMouse::OnMouseLeave()
{
	MouseState.bIsInsideWindow = false;
	MouseEventBuffer.push(UEvent(UEvent::EEventType::LEAVE, MouseState));
	TrimBuffer();
}

void UMouse::OnMouseEnter()
{
	MouseState.bIsInsideWindow = true;
	MouseEventBuffer.push(UEvent(UEvent::EEventType::ENTER, MouseState));
	TrimBuffer();
}

void UMouse::OnLeftPressed(int X, int Y)
{
	MouseState.bIsLeftPressed = true;
	MouseEventBuffer.push(UEvent(UEvent::EEventType::L_PRESS, MouseState));
	TrimBuffer();
}

void UMouse::OnLeftReleased(int X, int Y)
{
	MouseState.bIsLeftPressed = false;
	MouseEventBuffer.push(UEvent(UEvent::EEventType::L_PRESS, MouseState));
	TrimBuffer();
}

void UMouse::OnRightPressed(int X, int Y)
{
	MouseState.bIsRightPressed = true;
	MouseEventBuffer.push(UEvent(UEvent::EEventType::R_PRESS, MouseState));
	TrimBuffer();
}

void UMouse::OnRightReleased(int X, int Y)
{
	MouseState.bIsRightPressed = false;
	MouseEventBuffer.push(UEvent(UEvent::EEventType::R_PRESS, MouseState));
	TrimBuffer();
}

void UMouse::OnWheelUp(int X, int Y)
{
	MouseEventBuffer.push(UEvent(UEvent::EEventType::WHEEL_UP, MouseState));
	TrimBuffer();
}

void UMouse::OnWheelDown(int X, int Y)
{
	MouseEventBuffer.push(UEvent(UEvent::EEventType::WHEEL_DOWN, MouseState));
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