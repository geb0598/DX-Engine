#include "Containers/Containers.h"
#include "Types/Types.h"
#include "Window/Public/EventPublisher.h"
#include "Window/Public/Keyboard.h"

void UKeyboard::Flush()
{
	FlushKey();
	FlushChar();
}

void UKeyboard::FlushKey()
{
	KeyBuffer = TQueue<UEvent>();
}

void UKeyboard::FlushChar()
{
	CharBuffer = TQueue<char>();
}

bool UKeyboard::IsKeyPressed(uint8 KeyCode) const
{
	return KeyStates[KeyCode];
}

bool UKeyboard::IsKeyEmpty() const
{
	return KeyBuffer.empty();
}

bool UKeyboard::IsCharEmpty() const
{
	return CharBuffer.empty();
}

std::optional<UKeyboard::UEvent> UKeyboard::ReadKey()
{
	std::optional<UEvent> Event;
	if (!IsKeyEmpty())
	{
		Event = KeyBuffer.front();
		KeyBuffer.pop();
	}
	return Event;
}

std::optional<char> UKeyboard::ReadChar()
{
	std::optional<char> Char;
	if (!IsCharEmpty())
	{
		Char = CharBuffer.front();
		CharBuffer.pop();
	}
	return Char;
}

void UKeyboard::EnableAutoRepeat()
{
	bIsAutoRepeatEnabled = true;
}

void UKeyboard::DisableAutoRepeat()
{
	bIsAutoRepeatEnabled = false;
}

bool UKeyboard::IsAutoRepeatEnabled()
{
	return bIsAutoRepeatEnabled;
}

UEventPublisher<UKeyboard::UEvent>& UKeyboard::GetEventPublisher()
{
	return EventPublisher;
}

const UEventPublisher<UKeyboard::UEvent>& UKeyboard::GetEventPublisher() const
{
	return EventPublisher;
}

void UKeyboard::OnKeyPressed(uint8 KeyCode)
{
	KeyStates[KeyCode] = true;
	UEvent Event{ UEvent::EEventType::PRESS, KeyCode };
	KeyBuffer.push(Event);
	EventPublisher.Publish(Event);
	TrimBuffer(KeyBuffer);
}

void UKeyboard::OnKeyReleased(uint8 KeyCode)
{
	KeyStates[KeyCode] = false;
	UEvent Event{UEvent::EEventType::RELEASE, KeyCode};
	KeyBuffer.push(Event);
	EventPublisher.Publish(Event);
	TrimBuffer(KeyBuffer);
}

void UKeyboard::OnChar(char Char)
{
	CharBuffer.push(Char);
	TrimBuffer(CharBuffer);
}

void UKeyboard::ClearState()
{
	KeyStates.reset();
}

