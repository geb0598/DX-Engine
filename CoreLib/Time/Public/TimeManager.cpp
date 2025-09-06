#include "TimeManager.h"

/* private */

TimeManager::TimeManager() {}
TimeManager::~TimeManager() {}

/* public */

void TimeManager::Initialize()
{
	QueryPerformanceFrequency(&Frequency);
	QueryPerformanceCounter(&PreviousFrameTime);
}

void TimeManager::Update()
{
	PreviousFrameTime = CurrentFrameTime;
	QueryPerformanceCounter(&CurrentFrameTime);

	DeltaTime = static_cast<double>(CurrentFrameTime.QuadPart 
		- PreviousFrameTime.QuadPart)
		/ static_cast<double>(Frequency.QuadPart);
}

double TimeManager::GetDeltaTimeInSecond()
{
	return DeltaTime;
}

double TimeManager::GetDeltaTimeInMS()
{
	return DeltaTime * 1000.0;
}

double TimeManager::GetFPS()
{
	return 1 / DeltaTime;
}

// static

TimeManager& TimeManager::Instance()
{
	static TimeManager Instance;
	return Instance;
}