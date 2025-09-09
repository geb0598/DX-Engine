#include "Window/Public/WindowSettings.h"
#include <fstream>

void FWindowSettings::SaveToFile(const FString& FilePath) const
{
    std::ofstream file(FilePath);
    if (file.is_open())
    {
        file << "Width=" << Width << std::endl;
        file << "Height=" << Height << std::endl;
        file << "PosX=" << PosX << std::endl;
        file << "PosY=" << PosY << std::endl;
        file << "IsMaximized=" << (bIsMaximized ? 1 : 0) << std::endl;
        file << "WindowTitle=" << WindowTitle << std::endl;
        file << "RestoredWidth=" << RestoredWidth << std::endl;
        file << "RestoredHeight=" << RestoredHeight << std::endl;
        file << "RestoredPosX=" << RestoredPosX << std::endl;
        file << "RestoredPosY=" << RestoredPosY << std::endl;
        file.close();
    }
}

bool FWindowSettings::LoadFromFile(const FString& FilePath)
{
    std::ifstream file(FilePath);
    if (!file.is_open())
        return false;

    FString line;
    while (std::getline(file, line))
    {
        size_t pos = line.find('=');
        if (pos == FString::npos)
            continue;

        FString key = line.substr(0, pos);
        FString value = line.substr(pos + 1);

        if (key == "Width")
            Width = std::stoi(value);
        else if (key == "Height")
            Height = std::stoi(value);
        else if (key == "PosX")
            PosX = std::stoi(value);
        else if (key == "PosY")
            PosY = std::stoi(value);
        else if (key == "IsMaximized")
            bIsMaximized = (std::stoi(value) != 0);
        else if (key == "WindowTitle")
            WindowTitle = value;
        else if (key == "RestoredWidth")
            RestoredWidth = std::stoi(value);
        else if (key == "RestoredHeight")
            RestoredHeight = std::stoi(value);
        else if (key == "RestoredPosX")
            RestoredPosX = std::stoi(value);
        else if (key == "RestoredPosY")
            RestoredPosY = std::stoi(value);
    }

    file.close();
    return true;
}