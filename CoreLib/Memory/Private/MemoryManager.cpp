#include "../Public/MemoryManager.h"
#include "Utilities/Public/Logger.h"
#include <algorithm>

CMemoryManager& CMemoryManager::Instance() 
{
    static CMemoryManager instance;
    return instance;
}

void* CMemoryManager::Allocate(size_t size, const char* file, int line, const char* function) 
{
    if (size == 0) return nullptr;

    void* ptr = std::malloc(size);
    if (!ptr) {
        UE_LOG(Fatal, "Memory allocation failed! Size: %zu bytes", size);
        return nullptr;
    }

    {
        std::lock_guard<std::mutex> lock(Mutex);
        
        // 통계 업데이트
        TotalAllocationBytes += static_cast<uint32>(size);
        TotalAllocationCount++;
        CurrentAllocationBytes += static_cast<uint32>(size);
        CurrentAllocationCount++;
        
        // 최대 할당량 업데이트
        PeakAllocationBytes = max(PeakAllocationBytes, CurrentAllocationBytes);

        // 할당 정보 저장
        AllocationInfo info;
        info.Size = size;
        info.File = file ? file : "Unknown";
        info.Line = line;
        info.Function = function ? function : "Unknown";
        
        Allocations[ptr] = info;
    }

    UE_LOG(Debug, "Memory allocated: %p, Size: %zu bytes, Total: %u bytes (%u allocations)", 
           ptr, size, CurrentAllocationBytes, CurrentAllocationCount);

    return ptr;
}

void CMemoryManager::Deallocate(void* ptr) 
{
    if (!ptr) return;

    {
        std::lock_guard<std::mutex> lock(Mutex);
        
        auto it = Allocations.find(ptr);
        if (it != Allocations.end()) {
            size_t size = it->second.Size;
            
            // 통계 업데이트
            CurrentAllocationBytes -= static_cast<uint32>(size);
            CurrentAllocationCount--;
            
            // 할당 정보 제거
            Allocations.erase(it);
            
            UE_LOG(Debug, "Memory deallocated: %p, Size: %zu bytes, Remaining: %u bytes (%u allocations)", 
                   ptr, size, CurrentAllocationBytes, CurrentAllocationCount);
        } else {
            UE_LOG(Warning, "Attempting to deallocate untracked memory: %p", ptr);
        }
    }

    std::free(ptr);
}

void CMemoryManager::DumpAllocations() const 
{
    std::lock_guard<std::mutex> lock(Mutex);
    
    UE_LOG(Info, "=== Memory Allocation Dump ===");
    UE_LOG(Info, "Total Allocations: %u (%u bytes)", TotalAllocationCount, TotalAllocationBytes);
    UE_LOG(Info, "Current Allocations: %u (%u bytes)", CurrentAllocationCount, CurrentAllocationBytes);
    UE_LOG(Info, "Peak Memory Usage: %u bytes", PeakAllocationBytes);
    UE_LOG(Info, "Active Allocations: %zu", Allocations.size());
    
    if (!Allocations.empty()) {
        UE_LOG(Info, "--- Active Allocation Details ---");
        for (const auto& pair : Allocations) {
            const AllocationInfo& info = pair.second;
            UE_LOG(Info, "Ptr: %p, Size: %zu bytes, File: %s:%d, Function: %s", 
                   pair.first, info.Size, info.File, info.Line, info.Function);
        }
    }
    UE_LOG(Info, "===============================");
}

void CMemoryManager::ResetStats() 
{
    std::lock_guard<std::mutex> lock(Mutex);
    
    TotalAllocationBytes = CurrentAllocationBytes;
    TotalAllocationCount = CurrentAllocationCount;
    PeakAllocationBytes = CurrentAllocationBytes;
    
    UE_LOG(Info, "Memory statistics reset. Current state: %u bytes (%u allocations)", 
           CurrentAllocationBytes, CurrentAllocationCount);
}

// 전역 new 연산자 오버로딩
void* operator new(size_t size) 
{
    return CMemoryManager::Instance().Allocate(size);
}

void* operator new(size_t size, const char* file, int line, const char* function) 
{
    return CMemoryManager::Instance().Allocate(size, file, line, function);
}

void* operator new[](size_t size) 
{
    return CMemoryManager::Instance().Allocate(size);
}

void* operator new[](size_t size, const char* file, int line, const char* function) 
{
    return CMemoryManager::Instance().Allocate(size, file, line, function);
}

// 전역 delete 연산자 오버로딩
void operator delete(void* ptr) noexcept 
{
    CMemoryManager::Instance().Deallocate(ptr);
}

void operator delete(void* ptr, const char* file, int line, const char* function) noexcept 
{
    CMemoryManager::Instance().Deallocate(ptr);
}

void operator delete[](void* ptr) noexcept 
{
    CMemoryManager::Instance().Deallocate(ptr);
}

void operator delete[](void* ptr, const char* file, int line, const char* function) noexcept 
{
    CMemoryManager::Instance().Deallocate(ptr);
}