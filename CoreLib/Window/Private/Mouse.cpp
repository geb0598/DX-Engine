#include <optional>
#include <utility>
#include <memory>

#include <Windows.h>

#include "Containers/Containers.h"
#include "Window/Public/EventDispatcher.h"
#include "Window/Public/EventListener.h"
#include "Window/Public/Mouse.h"
#include "Types/Types.h"

void UMouse::Dispatch(UMouseEvent Event)
{
	auto it = ListenerArray.begin();
	while (it != ListenerArray.end())
	{
		if (auto Listener = it->lock())
		{
			Listener->OnEvent(Event);
			++it;
		}
		else
		{
			it = ListenerArray.erase(it);
		}
	}
}

void UMouse::Subscribe(std::shared_ptr<IEventListener<UMouseEvent>> Listener)
{
	ListenerArray.push_back(Listener);
}

void UMouse::UnSubscribe(std::shared_ptr<IEventListener<UMouseEvent>> Listener)
{
	ListenerArray.erase(
		std::remove_if(ListenerArray.begin(), ListenerArray.end(),
			[&](std::weak_ptr<IEventListener<UMouseEvent>> ListenerPtr) {
				auto CurrentListener = ListenerPtr.lock();
				return !CurrentListener || CurrentListener == Listener;
			}),
		ListenerArray.end()
	);
}

void UMouse::Flush()
{
	MouseEventBuffer = std::queue<UMouseEvent>();
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

std::optional<UMouseEvent> UMouse::Read()
{
	std::optional<UMouseEvent> Result;
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
	UMouseEvent Event{ UMouseEvent::EEventType::MOVE, MouseState };
	MouseEventBuffer.push(Event);
	Dispatch(Event);
	TrimBuffer();
}

void UMouse::OnMouseLeave()
{
	MouseState.bIsInsideWindow = false;
	UMouseEvent Event{ UMouseEvent::EEventType::LEAVE, MouseState };
	MouseEventBuffer.push(Event);
	Dispatch(Event);
	TrimBuffer();
}

void UMouse::OnMouseEnter()
{
	MouseState.bIsInsideWindow = true;
	UMouseEvent Event{ UMouseEvent::EEventType::ENTER, MouseState };
	MouseEventBuffer.push(Event);
	Dispatch(Event);
	TrimBuffer();
}

void UMouse::OnLeftPressed(int X, int Y)
{
	MouseState.bIsLeftPressed = true;
	UMouseEvent Event{ UMouseEvent::EEventType::L_PRESS, MouseState };
	MouseEventBuffer.push(Event);
	Dispatch(Event);
	TrimBuffer();
}

void UMouse::OnLeftReleased(int X, int Y)
{
	MouseState.bIsLeftPressed = false;
	UMouseEvent Event{ UMouseEvent::EEventType::L_PRESS, MouseState };
	MouseEventBuffer.push(Event);
	Dispatch(Event);
	TrimBuffer();
}

void UMouse::OnRightPressed(int X, int Y)
{
	MouseState.bIsRightPressed = true;
	UMouseEvent Event{ UMouseEvent::EEventType::R_PRESS, MouseState };
	MouseEventBuffer.push(Event);
	Dispatch(Event);
	TrimBuffer();
}

void UMouse::OnRightReleased(int X, int Y)
{
	MouseState.bIsRightPressed = false;
	UMouseEvent Event{ UMouseEvent::EEventType::R_PRESS, MouseState };
	MouseEventBuffer.push(Event);
	Dispatch(Event);
	TrimBuffer();
}

void UMouse::OnWheelUp(int X, int Y)
{
	UMouseEvent Event{ UMouseEvent::EEventType::WHEEL_UP, MouseState };
	MouseEventBuffer.push(Event);
	Dispatch(Event);
	TrimBuffer();
}

void UMouse::OnWheelDown(int X, int Y)
{
	UMouseEvent Event{ UMouseEvent::EEventType::WHEEL_DOWN, MouseState };
	MouseEventBuffer.push(Event);
	Dispatch(Event);
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