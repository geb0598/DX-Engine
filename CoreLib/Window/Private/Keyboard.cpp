#include "Containers/Containers.h"
#include "Types/Types.h"
#include "Window/Public/EventDispatcher.h"
#include "Window/Public/EventListener.h"
#include "Window/Public/Keyboard.h"

void UKeyboard::Dispatch(UKeyboardEvent Event)
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

void UKeyboard::Subscribe(std::shared_ptr<IEventListener<UKeyboardEvent>> Listener)
{
	ListenerArray.push_back(Listener);
}

void UKeyboard::UnSubscribe(std::shared_ptr<IEventListener<UKeyboardEvent>> Listener)
{
	ListenerArray.erase(
		std::remove_if(ListenerArray.begin(), ListenerArray.end(),
			[&](std::weak_ptr<IEventListener<UKeyboardEvent>> ListenerPtr) {
				auto CurrentListener = ListenerPtr.lock();
				return !CurrentListener || CurrentListener == Listener;
			}),
		ListenerArray.end()
	);
}

void UKeyboard::Flush()
{
	FlushKey();
	FlushChar();
}

void UKeyboard::FlushKey()
{
	KeyBuffer = TQueue<UKeyboardEvent>();
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

std::optional<UKeyboardEvent> UKeyboard::ReadKey()
{
	std::optional<UKeyboardEvent> Event;
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
	UKeyboardEvent Event{ UKeyboardEvent::EEventType::PRESS, KeyCode };
	KeyBuffer.push(Event);
	Dispatch(Event);
	TrimBuffer(KeyBuffer);
}

void UKeyboard::OnKeyReleased(uint8 KeyCode)
{
	KeyStates[KeyCode] = false;
	UKeyboardEvent Event{UKeyboardEvent::EEventType::RELEASE, KeyCode};
	KeyBuffer.push(Event);
	Dispatch(Event);
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

