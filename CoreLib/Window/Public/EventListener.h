#pragma once

template<typename TEvent>
class IEventListener
{
public:
	~IEventListener() = default;

	virtual void OnEvent(TEvent Event) = 0;
};
