#pragma once

#include <bitset>
#include <optional>

#include "Containers/Containers.h"
#include "Types/Types.h"


class UKeyboard 
{
public:
class UEvent
{
public:
	enum class EEventType
	{
		PRESS,
		RELEASE
	};

	UEvent(EEventType EventType, uint8 KeyCode) 
		: EventType(EventType), KeyCode(KeyCode) {}

	bool IsPress() const
	{
		return EventType == EEventType::PRESS;
	}

	bool IsRelease() const
	{
		return EventType == EEventType::RELEASE;
	}

	uint8 GetKeyCode() const
	{
		return KeyCode;
	}

private:
	EEventType EventType;
	uint8 KeyCode;
};

public:
	friend class UWindow;

	~UKeyboard() = default;

	UKeyboard() = default;

	UKeyboard(const UKeyboard&) = delete;
	UKeyboard(UKeyboard&&) = delete;

	UKeyboard& operator=(const UKeyboard&) = delete;
	UKeyboard& operator=(UKeyboard&&) = delete;

	void Flush();
	void FlushKey();
	void FlushChar();

	bool IsKeyPressed(uint8 KeyCode) const;
	bool IsKeyEmpty() const;
	bool IsCharEmpty() const;

	std::optional<UEvent> ReadKey();
	std::optional<char> ReadChar();

	void EnableAutoRepeat();
	void DisableAutoRepeat();
	bool IsAutoRepeatEnabled();

private:
	template<typename TElement>
	static void TrimBuffer(TQueue<TElement>& Buffer);

	static constexpr uint32 NUM_KEYS = 256u;
	static constexpr uint32 BUFFER_SIZE = 16u;

private:
	void OnKeyPressed(uint8 KeyCode);
	void OnKeyReleased(uint8 KeyCode);
	void OnChar(char Char);
	void ClearState();

private:
	bool bIsAutoRepeatEnabled;

	std::bitset<NUM_KEYS> KeyStates;
	TQueue<UEvent> KeyBuffer;
	TQueue<char> CharBuffer;
};

template<typename TElement>
inline void UKeyboard::TrimBuffer(TQueue<TElement>& Buffer)
{
	while (Buffer.size() > BUFFER_SIZE)
	{
		Buffer.pop();
	}
}
