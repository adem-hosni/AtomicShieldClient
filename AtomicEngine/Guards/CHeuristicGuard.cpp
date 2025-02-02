#include <fstream>
#include "StdInc.h"
#include "KernelCalls.hpp"
#include <fstream>
#include <windows.h>
#include <scanner.h>
std::vector<std::wstring> m_vSignatures;

CHeuristicGuard::CHeuristicGuard()
{
}

CHeuristicGuard::~CHeuristicGuard()
{
}

void CHeuristicGuard::Initialize()
{
}

std::string CHeuristicGuard::BuildSignatureParameters()
{
    std::stringstream ss;
    ss << (char)(m_vSignatures.size() + 1);

    for (const std::wstring& param : m_vSignatures)
    {
        ss << static_cast<char>(param.length() + 1);
        ss << std::string(param.begin(), param.end());
    }
    return ss.str();
}

void CHeuristicGuard::AddSignatures(std::map<std::string, std::vector<std::wstring>>& Signatures)
{
    for (auto& [name, vector] : Signatures)
    {
        for (auto& Signature : vector)
        {
            m_vSignatures.push_back(Signature);
        }
    }

    std::thread t(&CHeuristicGuard::SpawnScanProcess, this);
    t.detach();
}

void CHeuristicGuard::SpawnScanProcess()
{
    char szTempFilePath[MAX_PATH];
    GetTempPathA(MAX_PATH, szTempFilePath);
    sprintf(szTempFilePath, "s%d.tmp", SharedUtil::GenerateRandomNumber(32, 256));

    std::ofstream tempFile(szTempFilePath, std::ios::binary);
    if (!tempFile.is_open())
    {
        SharedUtil::AddDebugLog("Failed to open temporary file for writing.");
        return;
    }
    tempFile.write(reinterpret_cast<const char*>(scanner), sizeof(scanner));
    tempFile.close();

    char szCommandLine[512];
    memset(szCommandLine, 0, sizeof(szCommandLine));
    sprintf(szCommandLine, "\"%s\" --pid %d --sigs %s", szTempFilePath, GetCurrentProcessId(), BuildSignatureParameters().c_str());

    SECURITY_ATTRIBUTES sa = {0};
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.bInheritHandle = TRUE;
    sa.lpSecurityDescriptor = nullptr;

    HANDLE hReadPipe, hWritePipe;
    if (!CreatePipe(&hReadPipe, &hWritePipe, &sa, 0))
    {
        SharedUtil::AddDebugLog("Failed to create pipe. Error: 0x%llx", GetLastError());
    }

    STARTUPINFOA        startupInfo = {0};
    PROCESS_INFORMATION processInfo = {0};
    startupInfo.cb = sizeof(STARTUPINFOA);
    startupInfo.hStdOutput = hWritePipe;
    startupInfo.hStdError = hWritePipe;
    startupInfo.hStdInput = NULL;

    if (!CreateProcess(nullptr,                     // Path to the executable
                       szCommandLine,               // Command line arguments (nullptr if none)
                       nullptr,                     // Process security attributes
                       nullptr,                     // Thread security attributes
                       FALSE,                       // Inherit handles
                       CREATE_NO_WINDOW,            // Creation flags
                       nullptr,                     // Use parent's environment block
                       nullptr,                     // Use parent's starting directory
                       &startupInfo,                // Pointer to STARTUPINFO structure
                       &processInfo))
    {
        SharedUtil::AddDebugLog("Failed to create process. Error: 0x%llx", GetLastError());
    }

    CloseHandle(hWritePipe);

    WaitForSingleObject(processInfo.hProcess, INFINITE);

    char buffer[4096];
    memset(buffer, 0, sizeof(buffer));
    DWORD dwBytesRead;
    while (ReadFile(hReadPipe, buffer, sizeof(buffer) - 1, &dwBytesRead, NULL) && dwBytesRead > 0)
    {
        buffer[dwBytesRead] = '\0';            // Null-terminate the buffer
    }

    DWORD dwExitCode = 0;
    if (GetExitCodeProcess(processInfo.hProcess, &dwExitCode))
    {
        SharedUtil::AddDebugLog("Process finished with exit code: 0x%llx", dwExitCode);

        if (dwExitCode == 0x1c8)
        {
            g_pAtomicAntiCheat->NotifyDetection(eDetectionType::CHEAT_SIGNATURE_FOUND);
        }
    }
    else
    {
        SharedUtil::AddDebugLog("Failed to get exit code. Error: 0x%llx", GetLastError());
    }

    CloseHandle(hReadPipe);
    TerminateProcess(processInfo.hProcess, dwExitCode);
    CloseHandle(processInfo.hProcess);
    CloseHandle(processInfo.hThread);

    DeleteFileA(szTempFilePath);
}
