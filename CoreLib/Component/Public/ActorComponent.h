#pragma once

#include "Types/Types.h"

// NOTE: Forward declaration of UActor class
class AActor;

class UActorComponent
{
public:
	virtual ~UActorComponent() = default;

	UActorComponent(AActor* Actor);

	UActorComponent(const UActorComponent&) = delete;
	UActorComponent(UActorComponent&&) = delete;

	UActorComponent& operator=(const UActorComponent&) = delete;
	UActorComponent& operator=(UActorComponent&&) = delete;

	AActor* GetActor();
	const AActor* GetActor() const;

private:
	AActor* Actor;
};

#include "Actor/Actor.h"