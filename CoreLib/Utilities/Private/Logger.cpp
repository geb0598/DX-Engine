#include "../Public/Logger.h"

CLogger& CLogger::Instance() 
{
    static CLogger instance;
    static bool initialized = false;
    
    if (!initialized)
    {
        // ConsoleOutput을 기본으로 추가
        instance.AddOutput(std::make_unique<CConsoleOutput>());
        
        // JungleEngine.log 파일을 덮어쓰기 모드로 추가
        try {
            instance.AddOutput(std::make_unique<CFileOutput>("JungleEngine.log", false));
        }
        catch (const std::exception& e) {
        }
        
        initialized = true;
    }
    
    return instance;
}

void CLogger::AddOutput(std::unique_ptr<ILogOutput> output) 
{
    std::lock_guard<std::mutex> lock(Mutex);
    Outputs.push_back(std::move(output));
}

void CLogger::RemoveAllOutputs() 
{
    std::lock_guard<std::mutex> lock(Mutex);
    Outputs.clear();
}

FString CLogger::FormatLogMessage(const FString& message, ELogLevel level) const 
{
    return std::format("[{}][{}] {}\n",
        GetTimeString(),
        GetLogLevelString(level),
        message);

	// 스레드 ID 포함 버전
    //std::stringstream ss;
    //ss << std::this_thread::get_id();

    //return std::format("[{}][{}][{}] {}\n",
    //    GetTimeString(),
    //    GetLogLevelString(level),
    //    ss.str(),
    //    message);
}

FString CLogger::GetLogLevelString(ELogLevel level) const
{
	switch (level) 
    {
	case ELogLevel::DebugLevel:
		return "DEBUG";
	case ELogLevel::InfoLevel:
		return "INFO";
	case ELogLevel::WarningLevel:
		return "WARNING";
	case ELogLevel::ErrorLevel:
		return "ERROR";
	case ELogLevel::FatalLevel:
		return "FATAL";
	default:
		return "UNKNOWN";
	}
}

FString CLogger::GetTimeString() const 
{
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;

    std::tm tm;
    localtime_s(&tm, &time);

    return std::format("{:04d}-{:02d}-{:02d} {:02d}:{:02d}:{:02d}.{:03d}",
        tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
        tm.tm_hour, tm.tm_min, tm.tm_sec, ms.count());
}