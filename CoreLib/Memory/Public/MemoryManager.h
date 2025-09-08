#pragma once

#include <cstdlib>
#include <mutex>
#include <unordered_map>
#include <cstdint>
#include "Containers/Containers.h"
#include "Types/Types.h"

// 메모리 할당 정보 구조체
struct AllocationInfo 
{
    size_t Size;
    const char* File;
    int Line;
    const char* Function;
};

// Unreal Engine 스타일 메모리 관리자
class CMemoryManager 
{
public:
    static CMemoryManager& Instance();

    // 메모리 할당 및 해제
    void* Allocate(size_t size, const char* file = nullptr, int line = 0, const char* function = nullptr);
    void Deallocate(void* ptr);

    // 통계 정보
    uint32 GetTotalAllocationBytes() const { return TotalAllocationBytes; }
    uint32 GetTotalAllocationCount() const { return TotalAllocationCount; }
    uint32 GetCurrentAllocationBytes() const { return CurrentAllocationBytes; }
    uint32 GetCurrentAllocationCount() const { return CurrentAllocationCount; }
    uint32 GetPeakAllocationBytes() const { return PeakAllocationBytes; }

    // 메모리 덤프 (디버깅용)
    void DumpAllocations() const;
    void ResetStats();

private:
    CMemoryManager() = default;
    ~CMemoryManager() = default;

    // 복사/이동 방지
    CMemoryManager(const CMemoryManager&) = delete;
    CMemoryManager& operator=(const CMemoryManager&) = delete;

    // 통계 정보
    mutable std::mutex Mutex;
    uint32 TotalAllocationBytes = 0;    // 총 할당된 바이트 (누적)
    uint32 TotalAllocationCount = 0;    // 총 할당 횟수 (누적)
    uint32 CurrentAllocationBytes = 0;  // 현재 할당된 바이트
    uint32 CurrentAllocationCount = 0;  // 현재 할당 개수
    uint32 PeakAllocationBytes = 0;     // 최대 할당 바이트

    // 할당 추적
    std::unordered_map<void*, AllocationInfo> Allocations;
};

// 전역 new/delete 연산자 오버로딩
void* operator new(size_t size);
void* operator new(size_t size, const char* file, int line, const char* function);
void* operator new[](size_t size);
void* operator new[](size_t size, const char* file, int line, const char* function);

void operator delete(void* ptr) noexcept;
void operator delete(void* ptr, const char* file, int line, const char* function) noexcept;
void operator delete[](void* ptr) noexcept;
void operator delete[](void* ptr, const char* file, int line, const char* function) noexcept;

// 매크로 정의 (파일, 라인, 함수 정보 자동 추가)
#define NEW new(__FILE__, __LINE__, __FUNCTION__)
#define DELETE delete

// 메모리 추적을 위한 매크로
#define TRACK_MEMORY 1

#if TRACK_MEMORY
    #define MEMORY_NEW(size) CMemoryManager::Instance().Allocate(size, __FILE__, __LINE__, __FUNCTION__)
    #define MEMORY_DELETE(ptr) CMemoryManager::Instance().Deallocate(ptr)
#else
    #define MEMORY_NEW(size) std::malloc(size)
    #define MEMORY_DELETE(ptr) std::free(ptr)
#endif