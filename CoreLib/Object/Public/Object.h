#pragma once

#include "Containers/Containers.h"
#include "Types/Types.h"

class UObject
{
private:
	class UEngineStatics
	{
	public:
		static uint32 GenUUID();

		static uint32 NextUUID;
	};
public:
	~UObject();

	UObject();

	UObject(const UObject&) = delete;
	UObject(UObject&&) noexcept = default;

	UObject& operator=(const UObject&) = delete;
	UObject& operator=(UObject&&) noexcept = default;

	uint32 UUID;
	uint32 InternalIndex;
};

extern TArray<UObject*> GUDObjectArray;
extern TArray<uint32> GUDObjectFreeIndexArray;

extern uint32 TotalAllocationBytes;
extern uint32 TotalAllocationCount;
