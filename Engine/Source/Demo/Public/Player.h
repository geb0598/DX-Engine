#pragma once
#include "Actor/Public/Actor.h"

class APlayer : public AActor
{
	DECLARE_CLASS(APlayer, AActor)
public:
	APlayer();

	UClass* GetDefaultRootComponent() override;
	void InitializeComponents() override;
};
