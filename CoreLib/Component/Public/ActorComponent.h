#pragma once

#include "Types/Types.h"

// NOTE: Forward declaration of UActor class
class UActor;

class UActorComponent
{
public:
	virtual ~UActorComponent() = default;

	UActorComponent(UActor* Actor);

	UActorComponent(const UActorComponent&) = delete;
	UActorComponent(UActorComponent&&) = delete;

	UActorComponent& operator=(const UActorComponent&) = delete;
	UActorComponent& operator=(UActorComponent&&) = delete;

	UActor* GetActor();
	const UActor* GetActor() const;

private:
	UActor* Actor;
};
