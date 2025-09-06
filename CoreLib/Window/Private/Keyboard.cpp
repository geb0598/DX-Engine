#include "Containers/Containers.h"
#include "Types/Types.h"
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

void UKeyboard::OnKeyPressed(uint8 KeyCode)
{
	KeyStates[KeyCode] = true;
	KeyBuffer.push(UEvent(UEvent::EEventType::PRESS, KeyCode));
	TrimBuffer(KeyBuffer);
}

void UKeyboard::OnKeyReleased(uint8 KeyCode)
{
	KeyStates[KeyCode] = false;
	KeyBuffer.push(UEvent(UEvent::EEventType::RELEASE, KeyCode));
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

