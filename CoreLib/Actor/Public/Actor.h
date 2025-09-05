#pragma once

#include <cassert>
#include <memory>
#include <type_traits>

#include "Component/Public/ActorComponent.h"
#include "Containers/Containers.h"
#include "Types/Types.h"

class AActor
{
public:
	~AActor() = default;

	AActor();

	// TODO: AActor is non-copyable and non-movable, 
	//		 but it would become copyable and/or movable
	//		 according to implementation of UActorComponent
	AActor(const AActor&) = delete;
	AActor(AActor&&) = delete;

	AActor& operator=(const AActor&) = delete;
	AActor& operator=(AActor&&) = delete;

	template<typename TComponent, typename... TArgs>
	std::enable_if_t<std::is_base_of_v<UActorComponent, TComponent>>
		RegisterComponent(TArgs&&... Args);

	template<typename TComponent>
	std::enable_if_t<std::is_base_of_v<UActorComponent, TComponent>, TComponent>*
		GetComponent();

private:
	using ComponentKey = const void*;

	template<typename TComponent>
	static ComponentKey GetComponentKey();

	TMap<ComponentKey, std::unique_ptr<UActorComponent>> ComponentMap;
};

template<typename TComponent, typename ...TArgs>
inline std::enable_if_t<std::is_base_of_v<UActorComponent, TComponent>> AActor::RegisterComponent(TArgs && ...Args)
{
	assert(!ComponentMap.count(GetComponentKey<TComponent>()));

	ComponentMap.emplace(
		GetComponentKey<TComponent>(), 
		std::make_unique<TComponent>(std::forward<TArgs>(Args)...)
	);
}

template<typename TComponent>
inline std::enable_if_t<std::is_base_of_v<UActorComponent, TComponent>, TComponent>* AActor::GetComponent()
{
	// TODO
}

template<typename TComponent>
inline AActor::ComponentKey AActor::GetComponentKey()
{
	static uint8 _;
	return &_;
}
