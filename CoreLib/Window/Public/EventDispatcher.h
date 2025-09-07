#pragma once

#include <memory>

template<typename TEvent>
class IEventListener;

template<typename TEvent>
class IEventDispatcher
{
public:
	~IEventDispatcher() = default;

	virtual void Dispatch(TEvent Event) = 0;

	virtual void Subscribe(std::shared_ptr<IEventListener<TEvent>> Listener) = 0;
	virtual void UnSubscribe(std::shared_ptr<IEventListener<TEvent>> Listener) = 0;
};
