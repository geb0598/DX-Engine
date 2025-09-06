#include "Component/Public/ActorComponent.h"

UActorComponent::UActorComponent(AActor* Actor)
	: Actor(Actor)
{

}

AActor* UActorComponent::GetActor()
{
	return Actor;
}

const AActor* UActorComponent::GetActor() const
{
	return Actor;
}
