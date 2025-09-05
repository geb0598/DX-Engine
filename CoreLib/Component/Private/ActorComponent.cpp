#include "Component/Public/ActorComponent.h"

UActorComponent::UActorComponent(UActor* Actor)
	: Actor(Actor)
{

}

UActor* UActorComponent::GetActor()
{
	return Actor;
}

const UActor* UActorComponent::GetActor() const
{
	return Actor;
}
