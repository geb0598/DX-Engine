#include "Memory/Public/Memory.h"
#include <cstdlib>
#include "Utilities/Utilities.h"

uint32 FMemory::TotalAllocationBytes = 0;
uint32 FMemory::TotalAllocationCount = 0;

void* operator new(size_t size)
{
    void* ptr = std::malloc(size);
    FMemory::TotalAllocationBytes += static_cast<uint32>(size);
    FMemory::TotalAllocationCount++;
    return ptr;
}

void operator delete(void* ptr, size_t size) noexcept
{
    std::free(ptr);
    FMemory::TotalAllocationBytes -= static_cast<uint32>(size);
    FMemory::TotalAllocationCount--;
}
