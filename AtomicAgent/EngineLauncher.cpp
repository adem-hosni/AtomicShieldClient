#include "StdInc.h"
#include "EngineLauncher.h"

std::filesystem::path EngineLauncher::GetEnginePath()
{
    SharedUtil::AddDebugLog(__FUNCTION__);

    PWSTR                 programFilesPath = nullptr;
    std::filesystem::path basePath;

    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_ProgramFiles, 0, NULL, &programFilesPath)))
    {
        basePath = std::filesystem::path(programFilesPath) / "AtomicShield";
        CoTaskMemFree(programFilesPath);

        if (!std::filesystem::exists(basePath))
        {
            try
            {
                std::filesystem::create_directory(basePath);
                SharedUtil::AddDebugLog("Created AtomicShield directory in Program Files");
            }
            catch (const std::exception& e)
            {
                SharedUtil::AddDebugLog("Failed to create AtomicShield folder: %s", e.what());
            }
        }
    }
    else
    {
        basePath =  "AtomicShield";
    }

    return basePath / "AtomicSvc.exe";
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
    outFile.flush();

    outFile.close();
    return true;
}

EngineLauncher::eLaunchResult EngineLauncher::LaunchEngineProcess(const std::filesystem::path& EnginePath, HANDLE* pHandle)
{
    SharedUtil::AddDebugLog(__FUNCTION__);

    STARTUPINFOW        si = {sizeof(si)};
    PROCESS_INFORMATION pi = {};

    if (!CreateProcessW(EnginePath.c_str(), nullptr, nullptr, nullptr, FALSE, CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi))
    {
        DWORD error = GetLastError();
        SharedUtil::AddDebugLog("CreateProcess failed: ", error);
        return eLaunchResult::LAUNCH_ELEVATION_FAILED;
    }

    *pHandle = pi.hProcess;
    CloseHandle(pi.hThread);



    return eLaunchResult::SUCCESS;
}

int EngineLauncher::LoadEngineIntoLauncher(const std::filesystem::path& EnginePath, HANDLE hProcess, BYTE* pBuffer, size_t size)
{
    SharedUtil::AddDebugLog(__FUNCTION__);

    return ManualMapDll(hProcess, pBuffer, size);
}