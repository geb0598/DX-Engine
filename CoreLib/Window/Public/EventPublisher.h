#pragma once

#include <algorithm>
#include <functional>
#include <memory>

#include "Containers/Containers.h"

template<typename TEvent>
class [[deperecatd]] UEventPublisher
{
public:
	using FDelegate = std::function<void(const TEvent&)>;

	~UEventPublisher() = default;

	UEventPublisher() = default;

	UEventPublisher(const UEventPublisher&) = default;
	UEventPublisher(UEventPublisher&&) noexcept = default;

	UEventPublisher& operator=(const UEventPublisher&) = default;
	UEventPublisher& operator=(UEventPublisher&&) noexcept = default;

	void Publish(const TEvent& Event)
	{
		DelegateArray.erase(
			std::remove_if(DelegateArray.begin(), DelegateArray.end(),
				[](const auto& DelegatePair) {
					return DelegatePair.first.expired();
				}),
			DelegateArray.end()
		);

		for (const auto& [_, Delegate] : DelegateArray)
		{
			Delegate(Event);
		}
	}

	template<typename TSubscriber>
	void Subscribe(std::shared_ptr<TSubscriber> Subscriber, void (TSubscriber::* Delegate)(const TEvent&))
	{
		std::weak_ptr<TSubscriber> WeakSubscriber = Subscriber;

		FDelegate DelegateLambda = [WeakSubscriber, Delegate](const TEvent& Event) {
			if (auto Subscriber = WeakSubscriber.lock()) {
				(Subscriber.get()->*Delegate)(Event);
			}
		};
		
		DelegateArray.emplace_back(WeakSubscriber, DelegateLambda);
	}

private:
	TArray<std::pair<std::weak_ptr<void>, FDelegate>> DelegateArray;
};
