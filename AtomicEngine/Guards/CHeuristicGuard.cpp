#include "StdInc.h"
#include <fstream>

CHeuristicGuard::CHeuristicGuard()
{
}

CHeuristicGuard::~CHeuristicGuard()
{
}

std::vector<unsigned char> readBytesFromExe(const std::string& filePath)
{
    std::ifstream file(filePath, std::ios::in | std::ios::binary);

    if (!file)
    {
        SharedUtil::AddDebugLog("Unable to open the file!");
        return {};
    }

    file.seekg(0, std::ios::end);
    std::streampos fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<unsigned char> rawData(fileSize);

    file.read(reinterpret_cast<char*>(rawData.data()), fileSize);

    file.close();

    return rawData;
}

std::string CHeuristicGuard::BuildSignatureParameters()
{
    std::stringstream ss;
    ss << (char)(m_Signatures.size() + 1);

    for (const std::string& param : m_Signatures)
    {
        ss << static_cast<char>(param.length() + 1);
        ss << param.c_str();
    }
    return ss.str();
}

void CHeuristicGuard::Initialize()
{
}

void CHeuristicGuard::SpawnScanProcess()
{
    const char* szFilePath = "C:\\Users\\hosni\\Desktop\\memscn-main\\src\\x64\\Release\\scn.exe";

    char szCommandLine[512];
    memset(szCommandLine, 0, sizeof(szCommandLine));
    sprintf(szCommandLine, "\"%s\" --pid %d --sigs %s", szFilePath, GetCurrentProcessId(), BuildSignatureParameters().c_str());

    // Initialize variables
    STARTUPINFOA        startupInfo = {0};
    PROCESS_INFORMATION processInfo = {0};
    startupInfo.cb = sizeof(STARTUPINFOA);
    startupInfo.hStdOutput = NULL;
    startupInfo.hStdError = NULL;
    startupInfo.hStdInput = NULL;

    // Create the process
    if (!CreateProcessA(nullptr,                     // Path to the executable
                        szCommandLine,               // Command line arguments (nullptr if none)
                        nullptr,                     // Process security attributes
                        nullptr,                     // Thread security attributes
                        FALSE,                       // Inherit handles
                        CREATE_NO_WINDOW,            // Creation flags
                        nullptr,                     // Use parent's environment block
                        nullptr,                     // Use parent's starting directory
                        &startupInfo,                // Pointer to STARTUPINFO structure
                        &processInfo))
    {            // Pointer to PROCESS_INFORMATION structure
        SharedUtil::AddDebugLog("Failed to create process. Error: 0x%llx", GetLastError());
    }

    WaitForSingleObject(processInfo.hProcess, INFINITE);

    // Get the exit code
    DWORD dwExitCode = 0;
    if (GetExitCodeProcess(processInfo.hProcess, &dwExitCode))
    {
        SharedUtil::AddDebugLog("Process finished with exit code: 0x%llx", dwExitCode);
    }
    else
    {
        SharedUtil::AddDebugLog("Failed to get exit code. Error: 0x%llx", GetLastError());
    }

    TerminateProcess(processInfo.hProcess, dwExitCode);
    CloseHandle(processInfo.hProcess);
    CloseHandle(processInfo.hThread);
}

void CHeuristicGuard::AddSignatures(std::map<std::string, std::unordered_set<std::string>>& Signatures)
{
    for (auto& [name, vector] : Signatures)
    {
        for (auto& Signature : vector)
        {
            m_Signatures.insert(Signature);
        }
    }

    SpawnScanProcess();
}