#pragma once
#include <libloaderapi.h>
#include <string>

namespace FileSystemUtils
{
    std::wstring getCurrentDirectoryPath()
    {
        wchar_t currentPath[1024];
        GetModuleFileName(NULL, currentPath, 1024);
        std::wstring filePath(currentPath);
        filePath  = filePath.substr(0, filePath.find_last_of('\\')+1);
        return filePath;
    }
}
