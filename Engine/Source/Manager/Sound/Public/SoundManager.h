#pragma once
#include "Core/Public/Object.h"

UCLASS()
class USoundManager :public UObject
{
	GENERATED_BODY()
	DECLARE_SINGLETON_CLASS(USoundManager, UObject)

public:
	USoundManager();
};
