#pragma once

#include <optional>
#include <utility>

#include "Containers/Containers.h"
#include "Types/Types.h"


class UMouse 
{
public:
struct FMouseState
{
	bool bIsInsideWindow = false;
	bool bIsLeftPressed = false;
	bool bIsRightPressed = false;
	int32 X = 0;
	int32 Y = 0;
	int32 WheelDeltaCarry = 0;
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

	std::pair<int32, int32> GetPosition() const
	{
		return { MouseState.X, MouseState.Y };
	}

	int32 GetXPosition() const
	{
		return MouseState.X;
	}

	int32 GetYPosition() const
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
	int32 GetXPosition() const;
	int32 GetYPosition() const;

	// [추가] 마우스 위치 델타값 Getter
	int32 GetXPositionDelta() const;
	int32 GetYPositionDelta() const;

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

	// [추가] 델타 계산을 위한 이전 위치 저장
	int32 LastX = 0;
	int32 LastY = 0;
	int32 DeltaX = 0;
	int32 DeltaY = 0;
};
