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

    void SaveToFile(const FString& FilePath) const;
    bool LoadFromFile(const FString& FilePath);
};