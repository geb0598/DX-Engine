#pragma once

#include <windows.h>

class TimeManager
{
private:
	TimeManager();
	~TimeManager();

private:
	LARGE_INTEGER Frequency;
	LARGE_INTEGER CurrentFrameTime;
	LARGE_INTEGER PreviousFrameTime;

	double DeltaTime;

public:
	void Initialize();
	void Update();

	double GetDeltaTimeInSecond();
	double GetDeltaTimeInMS();
	double GetFPS();
	
	static TimeManager& Instance();
};