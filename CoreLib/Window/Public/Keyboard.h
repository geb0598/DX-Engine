#pragma once

#include <bitset>
#include <optional>

#include "Containers/Containers.h"
#include "EventDispatcher.h"
#include "Types/Types.h"

class UKeyboardEvent
{
public:
	enum class EEventType
	{
		PRESS,
		RELEASE
	};

	UKeyboardEvent(EEventType EventType, uint8 KeyCode) 
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

class UKeyboard : public IEventDispatcher<UKeyboardEvent>
{
public:
	friend class UWindow;

	~UKeyboard() = default;

	UKeyboard() = default;

	UKeyboard(const UKeyboard&) = delete;
	UKeyboard(UKeyboard&&) = delete;

	UKeyboard& operator=(const UKeyboard&) = delete;
	UKeyboard& operator=(UKeyboard&&) = delete;

	void Dispatch(UKeyboardEvent Event);

	void Subscribe(std::shared_ptr<IEventListener<UKeyboardEvent>> Listener);
	void UnSubscribe(std::shared_ptr<IEventListener<UKeyboardEvent>> Listener);

	void Flush();
	void FlushKey();
	void FlushChar();

	bool IsKeyPressed(uint8 KeyCode) const;
	bool IsKeyEmpty() const;
	bool IsCharEmpty() const;

	std::optional<UKeyboardEvent> ReadKey();
	std::optional<char> ReadChar();

	void EnableAutoRepeat();
	void DisableAutoRepeat();
	bool IsAutoRepeatEnabled();
	
private:
	template<typename TElement>
	static void TrimBuffer(TQueue<TElement>& Buffer);

	static constexpr uint32 NUM_KEYS = 256u;
	static constexpr uint32 BUFFER_SIZE = 16u;

	void OnKeyPressed(uint8 KeyCode);
	void OnKeyReleased(uint8 KeyCode);
	void OnChar(char Char);
	void ClearState();

	bool bIsAutoRepeatEnabled;

	std::bitset<NUM_KEYS> KeyStates;
	TQueue<UKeyboardEvent> KeyBuffer;
	TQueue<char> CharBuffer;

	TArray<std::weak_ptr<IEventListener<UKeyboardEvent>>> ListenerArray;
};

template<typename TElement>
inline void UKeyboard::TrimBuffer(TQueue<TElement>& Buffer)
{
	while (Buffer.size() > BUFFER_SIZE)
	{
		Buffer.pop();
	}
}
