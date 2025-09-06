#include <cassert>

#include "Object/Public/Object.h"

uint32 UObject::UEngineStatics::NextUUID = 0;

uint32 UObject::UEngineStatics::GenUUID()
{
	return NextUUID++;
}

UObject::~UObject()
{
	GUDObjectArray[InternalIndex] = nullptr;
	GUDObjectFreeIndexArray.push_back(InternalIndex);
}

UObject::UObject()
	: UUID(UEngineStatics::GenUUID())
{
	if (GUDObjectFreeIndexArray.empty())
	{
		InternalIndex = GUDObjectArray.size();
		GUDObjectArray.push_back(this);
	}
	else
	{
		uint32 Index = GUDObjectFreeIndexArray.back();
		GUDObjectFreeIndexArray.pop_back();

		assert(Index < GUDObjectArray.size() && !GUDObjectArray[Index]);
		
		InternalIndex = Index;
		GUDObjectArray[Index] = this;
	}
}
