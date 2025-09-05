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

private:
	UObject();
};

TArray<UObject*> GUDObjectArray;
TArray<uint32> GUDObjectFreeIndexArray;

uint32 TotalAllocationBytes;
uint32 TotalAllocationCount;
