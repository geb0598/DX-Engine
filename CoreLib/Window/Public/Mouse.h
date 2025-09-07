#pragma once

#include <optional>
#include <utility>

#include "Containers/Containers.h"
#include "EventPublisher.h"
#include "Types/Types.h"


class UMouse 
{
public:
struct FMouseState
{
	bool bIsInsideWindow = false;
	bool bIsLeftPressed = false;
	bool bIsRightPressed = false;
	int X = 0;
	int Y = 0;
	int WheelDeltaCarry = 0;
};

class UEvent
{
public:
	enum class EEventType
	{
		L_PRESS,
		L_RELEASE,
		R_PRESS,
		R_RELEASE,
		WHEEL_UP,
		WHEEL_DOWN,
		MOVE,
		ENTER,
		LEAVE
	};

	UEvent(EEventType EventType, FMouseState MouseState) : EventType(EventType), MouseState(MouseState) {}

	EEventType GetEventType() const
	{
		return EventType;
	}

	std::pair<int, int> GetPosition() const
	{
		return { MouseState.X, MouseState.Y };
	}

	int GetXPosition() const
	{
		return MouseState.X;
	}

	int GetYPosition() const
	{
		return MouseState.Y;
	}

	bool IsLeftPressed() const
	{
		return MouseState.bIsLeftPressed;
	}

	bool IsRightPressed() const
	{
		return MouseState.bIsRightPressed;
	}

private:
	EEventType EventType;
	FMouseState MouseState;
};

public:
	friend class UWindow;

	virtual ~UMouse() = default;

	UMouse() = default;

	UMouse(const UMouse&) = delete;
	UMouse& operator=(const UMouse&) = delete;

	UMouse(UMouse&&) = delete;
	UMouse& operator=(UMouse&&) = delete;

	void Flush();

	std::pair<int, int> GetPosition() const;
	int GetXPosition() const;
	int GetYPosition() const;

	UEventPublisher<UEvent>& GetEventPublisher();
	const UEventPublisher<UEvent>& GetEventPublisher() const;

	bool IsInsideWindow() const;
	bool IsLeftPressed() const;
	bool IsRightPressed() const;
	bool IsEmpty() const;

	std::optional<UEvent> Read();

private:
	static constexpr uint8 BUFFER_SIZE = 16u;

	void OnMouseMove(int X, int Y);
	void OnMouseLeave();
	void OnMouseEnter();
	void OnLeftPressed(int X, int Y);
	void OnLeftReleased(int X, int Y);
	void OnRightPressed(int X, int Y);
	void OnRightReleased(int X, int Y);
	void OnWheelUp(int X, int Y);
	void OnWheelDown(int X, int Y);
	void TrimBuffer();
	void OnWheelDelta(int X, int Y, int Delta);

	FMouseState MouseState;
	TQueue<UEvent> MouseEventBuffer;
		
	UEventPublisher<UEvent> EventPublisher;
};
