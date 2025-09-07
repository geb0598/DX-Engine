#pragma once

#include <cassert>
#include <memory>
#include <type_traits>

#include "Component/Public/ActorComponent.h"
#include "Containers/Containers.h"
#include "Types/Types.h"
#include "Object/Object.h"

class AActor : public UObject
{
public:
	virtual ~AActor() = default;

	AActor() = default;

	// TODO: AActor is non-copyable and non-movable, 
	//		 but it would become copyable and/or movable
	//		 according to implementation of UActorComponent
	AActor(const AActor&) = delete;
	AActor(AActor&&) = delete;

	AActor& operator=(const AActor&) = delete;
	AActor& operator=(AActor&&) = delete;

	template<typename TComponent, typename... TArgs>
	std::enable_if_t<std::is_base_of_v<UActorComponent, TComponent>>
		AddComponent(TArgs&&... Args);

	template<typename TComponent>
	std::enable_if_t<std::is_base_of_v<UActorComponent, TComponent>>
		RemoveComponent();

	template<typename TComponent>
	std::enable_if_t<std::is_base_of_v<UActorComponent, TComponent>, TComponent>*
		GetComponent();

	template<typename TComponent>
	std::enable_if_t<std::is_base_of_v<UActorComponent, TComponent>, bool>
		HasComponent();

private:
	using ComponentKey = const void*;

	template<typename TComponent>
	static ComponentKey GetComponentKey();

	TMap<ComponentKey, std::unique_ptr<UActorComponent>> ComponentMap;
};

template<typename TComponent, typename ...TArgs>
inline std::enable_if_t<std::is_base_of_v<UActorComponent, TComponent>> 
	AActor::AddComponent(TArgs&&... Args)
{
	assert(!HasComponent<TComponent>());

	ComponentMap.emplace(
		GetComponentKey<TComponent>(), 
		std::make_unique<TComponent>(std::forward<TArgs>(Args)...)
	);
}

template<typename TComponent>
inline std::enable_if_t<std::is_base_of_v<UActorComponent, TComponent>> AActor::RemoveComponent()
{
	assert(!HasComponent<TComponent>());
	
	ComponentMap.erase(GetComponentKey<TComponent>());
}

template<typename TComponent>
inline std::enable_if_t<std::is_base_of_v<UActorComponent, TComponent>, TComponent>* 
	AActor::GetComponent()
{
	if (HasComponent<TComponent>())
	{
		return static_cast<TComponent*>(ComponentMap[GetComponentKey<TComponent>()].get());
	}
	else
	{
		return nullptr;
	}
}

template<typename TComponent>
inline std::enable_if_t<std::is_base_of_v<UActorComponent, TComponent>, bool> 
	AActor::HasComponent()
{
	return ComponentMap.count(GetComponentKey<TComponent>());
}

template<typename TComponent>
inline AActor::ComponentKey AActor::GetComponentKey()
{
	static uint8 _;
	return &_;
}
