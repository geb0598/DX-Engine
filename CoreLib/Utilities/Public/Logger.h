#pragma once

#include <vector>
#include <string>
#include <memory>
#include <mutex>
#include <format>
#include <fstream>
#include <sstream>
#include <chrono>
#include <thread>
#include <windows.h>
#include "Containers/Containers.h"

// 로그 레벨
enum class ELogLevel {
	DebugLevel,      // DEBUG: 개발 중 상세 정보
	InfoLevel,       // INFO: 일반 정보
    WarningLevel,    // WARNING: 경고
	ErrorLevel,      // ERROR: 오류
	FatalLevel       // FATAL: 치명적인 오류
};

// 출력 인터페이스
class ILogOutput 
{
public:
    virtual ~ILogOutput() = default;
    virtual void Write(ELogLevel level, const FString& message) = 0;
};

// 디버그 창 출력
class CDebugOutput : public ILogOutput 
{
public:
    void Write(ELogLevel level, const FString& message) override 
    {
        OutputDebugStringA(message.c_str());
    }
};

// 파일 출력
class CFileOutput : public ILogOutput 
{
public:
    explicit CFileOutput(const FString& filename, bool append = false) 
    {
        if (append) 
        {
            File.open(filename, std::ios::out | std::ios::app);
        }
        else 
        {
            File.open(filename, std::ios::out | std::ios::trunc);
        }
        if (!File.is_open()) 
        {
            throw std::runtime_error("Failed to open log file");
        }
    }
    ~CFileOutput() 
    {
        if (File.is_open()) 
        {
            File.close();
        }
    }
    void Write(ELogLevel level, const FString& message) override 
    {
        if (File.is_open()) 
        {
            File << message;
            File.flush();
        }
    }

private:
    std::ofstream File;
};

// 콘솔 메시지 구조체
struct ConsoleMessage 
{
    ELogLevel level;
    FString message;
};

// 콘솔 UI 출력
class CConsoleOutput : public ILogOutput 
{
public:
    void Write(ELogLevel level, const FString& message) override
    {
        std::lock_guard<std::mutex> lock(GetConsoleMutex());
        GetConsoleMessages().push_back({level, message});

        // 너무 많은 메시지가 쌓이지 않도록 최대 1000개로 제한
        if (GetConsoleMessages().size() > 1000)
        {
            GetConsoleMessages().erase(GetConsoleMessages().begin());
        }
    }
    static std::vector<ConsoleMessage>& GetConsoleMessages() 
    {
        static std::vector<ConsoleMessage> messages;
        return messages;
    }
    static std::mutex& GetConsoleMutex() 
    {
        static std::mutex mutex;
        return mutex;
    }
};

class CLogger
{
public:
    static CLogger& Instance();

    // 로그 출력 함수들
    template<typename... Args>
    void DebugLevel(const FString& format, Args... args);

    template<typename... Args>
    void InfoLevel(const FString& format, Args... args);

    template<typename... Args>
    void WarningLevel(const FString& format, Args... args);

    template<typename... Args>
    void ErrorLevel(const FString& format, Args... args);

    template<typename... Args>
    void FatalLevel(const FString& format, Args... args);

    // 출력 대상 추가/제거
    void AddOutput(std::unique_ptr<ILogOutput> output);
    void RemoveAllOutputs();

    // 복사/이동 방지
    CLogger(const CLogger&) = delete;
    CLogger& operator=(const CLogger&) = delete;

private:
    CLogger() = default;

    template<typename... Args>
    void Log(ELogLevel level, const FString& format, Args... args);

    FString FormatLogMessage(const FString& message, ELogLevel level) const;
    FString GetLogLevelString(ELogLevel level) const;
    FString GetTimeString() const;

    std::mutex Mutex;
    std::vector<std::unique_ptr<ILogOutput>> Outputs;
};

// 템플릿 함수 구현
template<typename... Args>
void CLogger::DebugLevel(const FString& format, Args... args) 
{
    Log(ELogLevel::DebugLevel, format, std::forward<Args>(args)...);
}

template<typename... Args>
void CLogger::InfoLevel(const FString& format, Args... args) 
{
    Log(ELogLevel::InfoLevel, format, std::forward<Args>(args)...);
}

template<typename... Args>
void CLogger::WarningLevel(const FString& format, Args... args)
{
    Log(ELogLevel::WarningLevel, format, std::forward<Args>(args)...);
}

template<typename... Args>
void CLogger::ErrorLevel(const FString& format, Args... args) 
{
    Log(ELogLevel::ErrorLevel, format, std::forward<Args>(args)...);
}

template<typename... Args>
void CLogger::FatalLevel(const FString& format, Args... args)
{
    Log(ELogLevel::FatalLevel, format, std::forward<Args>(args)...);
}

template<typename... Args>
void CLogger::Log(ELogLevel level, const FString& format, Args... args) 
{
    try 
    {
        FString message;
        if constexpr (sizeof...(args) > 0) 
        {
            int size = std::snprintf(nullptr, 0, format.c_str(), args...);
            if (size > 0)
            {
                message.resize(size + 1);
                std::snprintf(message.data(), size + 1, format.c_str(), args...);
                message.resize(size);
            } 
            else 
            {
                message = format;
            }
        } 
        else 
        {
            message = format;
        }
        
        message = FormatLogMessage(message, level);

        std::lock_guard<std::mutex> lock(Mutex);
        for (const auto& output : Outputs) 
        {
            output->Write(level, message);
        }
    }
    catch (const std::exception& e)
    {
        // 포맷팅 실패 시 기본 에러 메시지 출력
        FString errorMessage = std::format("Logging failed: {}", e.what());
        std::lock_guard<std::mutex> lock(Mutex);
        for (const auto& output : Outputs)
        {
            output->Write(ELogLevel::ErrorLevel, errorMessage);
        }
    }
}

#define Debug ELogLevel::DebugLevel
#define Info ELogLevel::InfoLevel
#define Warning ELogLevel::WarningLevel
#define Error ELogLevel::ErrorLevel
#define Fatal ELogLevel::FatalLevel

// UE_LOG 매크로 정의
#define UE_LOG(level, format, ...) \
    do { \
        switch(level) { \
            case ELogLevel::DebugLevel: \
                CLogger::Instance().DebugLevel(format, ##__VA_ARGS__); \
                break; \
            case ELogLevel::InfoLevel: \
                CLogger::Instance().InfoLevel(format, ##__VA_ARGS__); \
                break; \
            case ELogLevel::WarningLevel: \
                CLogger::Instance().WarningLevel(format, ##__VA_ARGS__); \
                break; \
            case ELogLevel::ErrorLevel: \
                CLogger::Instance().ErrorLevel(format, ##__VA_ARGS__); \
                break; \
            case ELogLevel::FatalLevel: \
                CLogger::Instance().FatalLevel(format, ##__VA_ARGS__); \
                break; \
        } \
    } while(0)