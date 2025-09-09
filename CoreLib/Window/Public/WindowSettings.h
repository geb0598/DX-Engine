#pragma once
#include "Types/Types.h"
#include "Containers/Containers.h"
#include <Windows.h>

struct FWindowSettings
{
    int32 Width = 1024;
    int32 Height = 1024;
    int32 PosX = 100;
    int32 PosY = 100;
    bool bIsMaximized = false;
    FString WindowTitle = "Jungle Engine";
    
    // 최대화되기 전의 크기와 위치 저장
    int32 RestoredWidth = 1024;
    int32 RestoredHeight = 1024;
    int32 RestoredPosX = 100;
    int32 RestoredPosY = 100;

    void SaveToFile(const FString& FilePath) const;
    bool LoadFromFile(const FString& FilePath);
};