#include <string>
#include <Windows.h>
#include <Psapi.h>
#include <algorithm>
#include <random>
#include "SharedUtil.h"
#include "RuntimeImportResolver.h"
#include <TlHelp32.h>

bool SharedUtil::TerminateProcess(DWORD dwPID)
{
    DWORD  dwDesiredAccess = PROCESS_TERMINATE;
    bool   bInheritHandle = FALSE;
    HANDLE hProcess = OpenProcess(dwDesiredAccess, bInheritHandle, dwPID);
    if (hProcess == NULL)
        return FALSE;

    bool result = ::TerminateProcess(hProcess, 0);

    CloseHandle(hProcess);

    return result;
}

bool SharedUtil::FindStringIC(const std::string& strHaystack, const std::string& strNeedle)
{
    auto it = std::search(strHaystack.begin(), strHaystack.end(), strNeedle.begin(), strNeedle.end(),
                          [](char ch1, char ch2) { return std::toupper(ch1) == std::toupper(ch2); });
    return (it != strHaystack.end());
}

int SharedUtil::GetProcessID(const char* szProcessName)
{
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE)
    {
        return 0;            // Error handling
    }

    PROCESSENTRY32 pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32);

    if (!Process32First(hSnapshot, &pe32))
    {
        CloseHandle(hSnapshot);
        return 0;            // Error handling
    }

    do
    {
        if (strcmp(pe32.szExeFile, szProcessName) == 0)
        {
            CloseHandle(hSnapshot);
            return pe32.th32ProcessID;
        }
    } while (Process32Next(hSnapshot, &pe32));

    CloseHandle(hSnapshot);
    return NULL;            // Process not found
}

int SharedUtil::GetFivemProcessID()
{
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE)
    {
        return 0;            // Error handling
    }

    PROCESSENTRY32 pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32);

    if (!Process32First(hSnapshot, &pe32))
    {
        CloseHandle(hSnapshot);
        return 0;            // Error handling
    }

    auto IsFivemProcess = [](const std::string& strProcessName) -> bool
    {
        // List of base names
        static std::vector<std::string> baseNames = {"FiveM", "NVN", "GvR"};

        static std::vector<std::string> suffixes = {"_GTAProcess.exe", "_GameProcess.exe"};

        for (const auto& base : baseNames)
        {
            if (strProcessName.starts_with(base))
            {
                for (const auto& suffix : suffixes)
                {
                    if (strProcessName.ends_with(suffix))
                    {
                        return true;
                    }
                }
            }
        }
        return false;
    };

    do
    {
        if (IsFivemProcess(pe32.szExeFile))
        {
            CloseHandle(hSnapshot);
            return pe32.th32ProcessID;
        }
    } while (Process32Next(hSnapshot, &pe32));

    CloseHandle(hSnapshot);
    return NULL;            // Process not found
}

int SharedUtil::GenerateRandomNumber(int min, int max)
{
    std::random_device              rd;
    std::mt19937                    gen(rd());
    std::uniform_int_distribution<> distrib(min, max);
    return distrib(gen);
}

std::string SharedUtil::GenerateRandomString(int iLength)
{
    const static std::string              characters = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    std::random_device                    rd;                         // Seed for the random number generator
    std::mt19937                          generator(rd());            // Mersenne Twister random number engine
    std::uniform_int_distribution<size_t> distribution(0, characters.size() - 1);

    std::string randomString;
    for (size_t i = 0; i < iLength; ++i)
    {
        randomString += characters[distribution(generator)];
    }

    return randomString;
}

bool SharedUtil::IsRunningAsAdministator()
{
    BOOL                     bIsAdmin = FALSE;
    PSID                     adminGroup = NULL;
    SID_IDENTIFIER_AUTHORITY ntAuthority = SECURITY_NT_AUTHORITY;

    if (AllocateAndInitializeSid(&ntAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &adminGroup))
    {
        CheckTokenMembership(NULL, adminGroup, &bIsAdmin);
        FreeSid(adminGroup);
    }
    return bIsAdmin;
}

const char* SharedUtil::GetParentProcessName()
{
    ULONG_PTR pbi[6];
    ULONG     ulSize = 0;
    DWORD     dwPID = 0x0;
    LONG(WINAPI * NtQueryInformationProcess)
    (HANDLE ProcessHandle, ULONG ProcessInformationClass, PVOID ProcessInformation, ULONG ProcessInformationLength, PULONG ReturnLength);
    *(FARPROC*)&NtQueryInformationProcess = GetProcAddress(LoadLibraryA("ntdll.DLL"), "NtQueryInformationProcess");
    if (NtQueryInformationProcess)
    {
        if (NtQueryInformationProcess(GetCurrentProcess(), 0, &pbi, sizeof(pbi), &ulSize) >= 0 && ulSize == sizeof(pbi))
        {
            dwPID = pbi[5];
            HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, dwPID);
            if (!hProcess)
            {
                CloseHandle(hProcess);
                return "";
            }
            TCHAR szProcessName[MAX_PATH];
            if (!GetModuleFileNameEx(hProcess, 0, szProcessName, sizeof(szProcessName)))
                return "";
            return szProcessName;
        }
    }
    return "";
}

void SharedUtil::AddDebugLog(const char* szLog, ...)
{
    char localAppDataPath[MAX_PATH];
    if (FAILED(SHGetFolderPathA(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, localAppDataPath)))
        return;

    char szLogDirectory[MAX_PATH];
    memset(szLogDirectory, 0, MAX_PATH);
    sprintf(szLogDirectory, "%s\\AtomicShield", localAppDataPath);

    CreateDirectory(szLogDirectory, NULL);

    char szNewFile[600];
    memset(szNewFile, 0, sizeof(szNewFile));
    sprintf(szNewFile, "%s\\Trace.logs", szLogDirectory);
    static bool bOnce = false;
    if (!bOnce)
    {
        FILE* hFile = fopen(szNewFile, "rb");
        if (hFile)
        {
            fclose(hFile);
            DeleteFileA(szNewFile);
        }
        bOnce = true;
    }
    FILE* hFile = fopen(szNewFile, "a+");

    if (hFile)
    {
        time_t t = std::time(0);
        tm*    now = std::localtime(&t);
        char   szTimestamp[600];
        memset(szTimestamp, 0, sizeof(szTimestamp));
        sprintf(szTimestamp, "[%d:%d:%d] %s\n", now->tm_hour, now->tm_min, now->tm_sec, szLog);
        va_list args;
        va_start(args, szLog);
        vprintf(szTimestamp, args);
        vfprintf(hFile, szTimestamp, args);
        va_end(args);
        fclose(hFile);
    }
    else
    {
        printf("Failed to open file %s\n", szNewFile);
    }
}

bool SharedUtil::GetDebugLogs(std::string& szLog)
{
    char localAppDataPath[MAX_PATH];
    if (FAILED(SHGetFolderPathA(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, localAppDataPath)))
        return false;
    char szLogDirectory[MAX_PATH];
    memset(szLogDirectory, 0, MAX_PATH);
    sprintf(szLogDirectory, "%s\\AtomicShield", localAppDataPath);
    char szNewFile[600];
    memset(szNewFile, 0, sizeof(szNewFile));
    sprintf(szNewFile, "%s\\Trace.logs", szLogDirectory);
    FILE* hFile = fopen(szNewFile, "rb");
    if (hFile)
    {
        fseek(hFile, 0, SEEK_END);
        long fileSize = ftell(hFile);
        fseek(hFile, 0, SEEK_SET);
        szLog.resize(fileSize);
        fread(&szLog[0], 1, fileSize, hFile);
        fclose(hFile);
        return true;
    }
    return false;
}

std::string SharedUtil::GetKnownDirectory(const KNOWNFOLDERID fid)
{
    PWSTR path = nullptr;
    char  szProgramDataDir[MAX_PATH];
    memset(szProgramDataDir, 0, sizeof(szProgramDataDir));

    HRESULT result = SHGetKnownFolderPath(fid, 0, NULL, &path);
    if (!FAILED(result))
    {
        wcstombs(szProgramDataDir, path, MAX_PATH);
        //CoTaskMemFree(path);
    }
    return std::string(szProgramDataDir);
}

bool SharedUtil::SetPrivilege(LPCTSTR lpszPrivilege)
{
    HANDLE           hToken;
    TOKEN_PRIVILEGES tkp;

    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
        return FALSE;

    if (!LookupPrivilegeValue(NULL, lpszPrivilege, &tkp.Privileges[0].Luid))
    {
        CloseHandle(hToken);
        return FALSE;
    }

    tkp.PrivilegeCount = 1;
    tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    if (!AdjustTokenPrivileges(hToken, FALSE, &tkp, sizeof(tkp), NULL, NULL))
    {
        CloseHandle(hToken);
        return FALSE;
    }

    if (GetLastError() == ERROR_NOT_ALL_ASSIGNED)
    {
        CloseHandle(hToken);
        return FALSE;
    }

    CloseHandle(hToken);
    return TRUE;
}

std::string SharedUtil::Base64Encode(std::string data)
{
    static const char base64Chars[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789+/";

    std::string encoded;
    int         val = 0, valb = -6;
    for (unsigned char c : data)
    {
        val = (val << 8) + c;
        valb += 8;
        while (valb >= 0)
        {
            encoded.push_back(base64Chars[(val >> valb) & 0x3F]);
            valb -= 6;
        }
    }
    if (valb > -6)
    {
        encoded.push_back(base64Chars[((val << 8) >> (valb + 8)) & 0x3F]);
    }
    while (encoded.size() % 4)
    {
        encoded.push_back('=');
    }
    return encoded;
}

std::wstring SharedUtil::Base64Encode(std::wstring data)
{
    static const wchar_t base64Chars[] =
        L"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        L"abcdefghijklmnopqrstuvwxyz"
        L"0123456789+/";

    std::wstring encoded;
    int          val = 0, valb = -6;
    for (wchar_t c : data)
    {
        val = (val << 8) + c;
        valb += 8;
        while (valb >= 0)
        {
            encoded.push_back(base64Chars[(val >> valb) & 0x3F]);
            valb -= 6;
        }
    }
    if (valb > -6)
    {
        encoded.push_back(base64Chars[((val << 8) >> (valb + 8)) & 0x3F]);
    }
    while (encoded.size() % 4)
    {
        encoded.push_back(L'=');
    }
    return encoded;
}

std::string SharedUtil::Base64Decode(std::string encoded_string)
{
    static const std::string base64_chars =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789+/";

    auto is_base64 = [](unsigned char c) { return (isalnum(c) || (c == '+') || (c == '/')); };

    size_t        in_len = encoded_string.size();
    size_t        i = 0;
    size_t        j = 0;
    int           in_ = 0;
    unsigned char char_array_4[4], char_array_3[3];
    std::string   decoded_string;

    while (in_len-- && (encoded_string[in_] != '=') && is_base64(encoded_string[in_]))
    {
        char_array_4[i++] = encoded_string[in_];
        in_++;
        if (i == 4)
        {
            for (i = 0; i < 4; i++)
            {
                char_array_4[i] = base64_chars.find(char_array_4[i]);
            }

            char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
            char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
            char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

            for (i = 0; i < 3; i++)
            {
                decoded_string += char_array_3[i];
            }
            i = 0;
        }
    }

    if (i)
    {
        for (j = i; j < 4; j++)
        {
            char_array_4[j] = 0;
        }

        for (j = 0; j < 4; j++)
        {
            char_array_4[j] = base64_chars.find(char_array_4[j]);
        }

        char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
        char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
        char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

        for (j = 0; j < (i - 1); j++)
        {
            decoded_string += char_array_3[j];
        }
    }

    return decoded_string;
}

void SharedUtil::SetRegistryIntValue(const char* ss,const char* szKey, int iValue)
{
    HKEY hKey;

    if (RegCreateKeyExA(HKEY_CURRENT_USER, "Software\\AtomicShield", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL) ==
        ERROR_SUCCESS)
    {
        RegSetValueExA(hKey, szKey, 0, REG_DWORD, (const BYTE*)&iValue, sizeof(iValue));
        RegCloseKey(hKey);
    }
    else
    {
        AddDebugLog("Failed to create or open registry key.");
    }
}

bool Services::IsDriverRunning(__in const std::wstring& serviceName)
{
    if (serviceName.size() == 0)
        return false;

    SC_HANDLE hSCManager = OpenSCManager(nullptr, nullptr, SC_MANAGER_ENUMERATE_SERVICE);

    if (!hSCManager)
    {
        Logger::logfw(Warning, L"Failed to open SCM: %d", GetLastError());
        return false;
    }

    // open driver service
    SC_HANDLE hService = OpenService(hSCManager, serviceName.c_str(), SERVICE_QUERY_STATUS);

    if (!hService)
    {
        Logger::logfw(Info, L"Failed to open service %s: %d (this is not an error)", serviceName.c_str(), GetLastError());
        CloseServiceHandle(hSCManager);
        return false;
    }

    SERVICE_STATUS status;

    if (!QueryServiceStatus(hService, &status))            // query the service status
    {
        Logger::logfw(Warning, L"Failed to query service status: %d", GetLastError());
        CloseServiceHandle(hService);
        CloseServiceHandle(hSCManager);
        return false;
    }

    CloseServiceHandle(hService);
    CloseServiceHandle(hSCManager);

    return (status.dwCurrentState == SERVICE_RUNNING);
}


static void AddLogFmt(const char* prefix, const std::string& msg)
{
    std::stringstream ss;
    ss << "[" << prefix << "] " << msg;
    SharedUtil::AddDebugLog(ss.str());
}

// Constructor (no EvidenceLocker dependency)
Debugger::AntiDebug::AntiDebug(Settings* s) : DetectionThread(nullptr), Config(s)
{
    if (s == nullptr)
    {
        AddLogFmt("Warning", "Settings pointer is nullptr in AntiDebug::AntiDebug()");
    }

    if (!PreventWindowsDebuggers())
    {
        AddLogFmt("Warning", "PreventWindowsDebuggers failed in AntiDebug::AntiDebug()");
    }

    // common debugger process names (kept for possible expansion)
    CommonDebuggerProcesses.push_back(L"x64dbg.exe");
    CommonDebuggerProcesses.push_back(L"CheatEngine.exe");
    CommonDebuggerProcesses.push_back(L"idaq64.exe");
    CommonDebuggerProcesses.push_back(L"cheatengine-x86_64-SSE4-AVX2.exe");
    CommonDebuggerProcesses.push_back(L"kd.exe");
    CommonDebuggerProcesses.push_back(L"DbgX.Shell.exe");
}

Debugger::AntiDebug::~AntiDebug()
{
    if (this->DetectionThread)
    {
        // assume Thread has these methods - if not, replace with appropriate shutdown logic
        this->DetectionThread->SignalShutdown(TRUE);
        this->DetectionThread->JoinThread();
        this->DetectionThread.reset();
    }
}

Thread* Debugger::AntiDebug::GetDetectionThread() const
{
    return this->DetectionThread.get();
}
Settings* Debugger::AntiDebug::GetSettings() const
{
    return this->Config;
}

void Debugger::AntiDebug::StartAntiDebugThread()
{
    if (this->GetSettings() != nullptr)
    {
        // If your Settings has a flag to toggle anti-debug, honour it (field name may differ)
        // Example: if (!this->GetSettings()->bUseAntiDebugging) { AddLogFmt("Info","Anti-Debugger disabled"); return; }
    }

    // Create the thread (assumes Thread constructor signature remains present in codebase)
    try
    {
        this->DetectionThread = std::make_unique<Thread>((LPTHREAD_START_ROUTINE)Debugger::AntiDebug::CheckForDebugger, (LPVOID)this, true, false);

        {
            std::stringstream ss;
            ss << "Created Debugger detection thread";
            AddLogFmt("Info", ss.str());
        }
    }
    catch (...)
    {
        AddLogFmt("Err", "Failed to create AntiDebug detection thread (exception)");
    }
}

void Debugger::AntiDebug::CheckForDebugger(LPVOID AD)
{
    if (AD == nullptr)
    {
        AddLogFmt("Err", "AntiDbg class was NULL @ CheckForDebugger");
        return;
    }

    Debugger::AntiDebug* AntiDbg = reinterpret_cast<Debugger::AntiDebug*>(AD);
    AddLogFmt("Info", "Started Debugger detection thread");

    bool      MonitoringDebugger = true;
    const int MonitorLoopDelayMS = 1000;

    while (MonitoringDebugger)
    {
        if (AntiDbg->DetectionThread && AntiDbg->DetectionThread->IsShutdownSignalled())
        {
            AddLogFmt("Info", "Shutting down Debugger detection thread");
            break;
        }

        // spawn thread to check hardware registers (it suspends other threads)
        HANDLE CheckHardwareRegistersThread = CreateThread(0, 0, (LPTHREAD_START_ROUTINE)_IsHardwareDebuggerPresent, (LPVOID)AntiDbg, 0, 0);

        if (CheckHardwareRegistersThread == INVALID_HANDLE_VALUE || CheckHardwareRegistersThread == NULL)
        {
            std::stringstream ss;
            ss << "Failed to create thread to call _IsHardwareDebuggerPresent: " << GetLastError();
            AddLogFmt("Warning", ss.str());
        }
        else
        {
            CloseHandle(CheckHardwareRegistersThread);
        }

        AntiDbg->RunDetectionFunctions();

        if (AntiDbg->IsDBK64DriverLoaded())
        {
            AddLogFmt("Detection", "DBK64 driver detected (IsDriverRunning returned true)");
            AntiDbg->AddFlagged(DetectionFlags::DEBUG_DBK64_DRIVER);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(MonitorLoopDelayMS));
    }
}

void Debugger::AntiDebug::_IsHardwareDebuggerPresent(LPVOID AD)
{
    if (AD == nullptr)
    {
        AddLogFmt("Err", "AntiDbg class was NULL @ _IsHardwareDebuggerPresent");
        return;
    }

    Debugger::AntiDebug* AntiDbg = reinterpret_cast<Debugger::AntiDebug*>(AD);

    THREADENTRY32 te32;
    te32.dwSize = sizeof(THREADENTRY32);

    HANDLE hThreadSnap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (hThreadSnap == INVALID_HANDLE_VALUE)
    {
        std::stringstream ss;
        ss << "unable to create toolhelp snapshot: " << GetLastError();
        AddLogFmt("Err", ss.str());
        return;
    }

    DWORD currentProcessID = GetCurrentProcessId();

    if (Thread32First(hThreadSnap, &te32))
    {
        do
        {
            if (te32.th32OwnerProcessID == currentProcessID && te32.th32ThreadID != GetCurrentThreadId())
            {
                HANDLE hThread = OpenThread(THREAD_GET_CONTEXT | THREAD_SUSPEND_RESUME | THREAD_QUERY_INFORMATION, FALSE, te32.th32ThreadID);

                if (hThread == NULL)
                {
                    std::stringstream ss;
                    ss << "unable to OpenThread id: " << te32.th32ThreadID;
                    AddLogFmt("Warning", ss.str());
                    continue;
                }

                SuspendThread(hThread);

                CONTEXT context;
                context.ContextFlags = CONTEXT_DEBUG_REGISTERS;

                if (GetThreadContext(hThread, &context))
                {
                    if (context.Dr0 || context.Dr1 || context.Dr2 || context.Dr3 || context.Dr6 || context.Dr7)
                    {
                        AddLogFmt("Detection", "Hardware debug registers found on a thread (possible hardware debugger)");
                        ResumeThread(hThread);
                        AntiDbg->AddFlagged(DetectionFlags::DEBUG_HARDWARE_REGISTERS);
                        CloseHandle(hThreadSnap);
                        CloseHandle(hThread);
                        return;
                    }
                }
                else
                {
                    std::stringstream ss;
                    ss << "GetThreadContext failed: " << GetLastError();
                    AddLogFmt("Err", ss.str());
                    ResumeThread(hThread);
                    CloseHandle(hThread);
                    continue;
                }

                ResumeThread(hThread);
                CloseHandle(hThread);
            }
        } while (Thread32Next(hThreadSnap, &te32));
    }
    else
    {
        std::stringstream ss;
        ss << "Thread32First failed: " << GetLastError();
        AddLogFmt("Err", ss.str());
    }

    CloseHandle(hThreadSnap);
}

// Run all detection functions added via AddDetectionFunction
void Debugger::AntiDebug::RunDetectionFunctions()
{
    std::lock_guard<std::mutex> lock(this->DetectionRoutineMutex);

    for (const auto& func : this->DetectionFunctionList)
    {
        DetectionFlags DetectedMethod = DetectionFlags::NONE;

        if ((DetectedMethod = func()) != DetectionFlags::NONE)
        {
            // Only keep flags > EXECUTION_ERROR (mirrors original behavior)
            if (static_cast<int>(DetectedMethod) > static_cast<int>(DetectionFlags::EXECUTION_ERROR))
            {
                this->AddFlagged(DetectedMethod);
                {
                    std::stringstream ss;
                    ss << "Debugger flag detected: " << static_cast<int>(DetectedMethod);
                    AddLogFmt("Info", ss.str());
                }
            }
        }
    }
}

bool Debugger::AntiDebug::PreventWindowsDebuggers()
{
    HMODULE ntdll = GetModuleHandleA("ntdll.dll");

    if (!ntdll)
    {
        AddLogFmt("Err", "Failed to find ntdll.dll in PreventWindowsDebuggers");
        return false;
    }

    DWORD dwOldProt = 0;

    UINT64 DbgBreakpoint_Address = (UINT64)GetProcAddress(ntdll, "DbgBreakPoint");
    UINT64 DbgUiRemoteBreakin_Address = (UINT64)GetProcAddress(ntdll, "DbgUiRemoteBreakin");

    if (DbgBreakpoint_Address)
    {
        if (VirtualProtect((LPVOID)DbgBreakpoint_Address, 1, PAGE_EXECUTE_READWRITE, &dwOldProt))
        {
            __try
            {
                *(BYTE*)DbgBreakpoint_Address = 0xC3;            // ret
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                AddLogFmt("Err", "Failed to patch DbgBreakPoint");
                return false;
            }
            VirtualProtect((LPVOID)DbgBreakpoint_Address, 1, dwOldProt, &dwOldProt);
        }
    }

    if (DbgUiRemoteBreakin_Address)
    {
        if (VirtualProtect((LPVOID)DbgUiRemoteBreakin_Address, 1, PAGE_EXECUTE_READWRITE, &dwOldProt))
        {
            __try
            {
                *(BYTE*)DbgUiRemoteBreakin_Address = 0xC3;            // ret
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                AddLogFmt("Err", "Failed to patch DbgUiRemoteBreakin");
                return false;
            }
            VirtualProtect((LPVOID)DbgUiRemoteBreakin_Address, 1, dwOldProt, &dwOldProt);
        }
    }

    return true;
}

bool Debugger::AntiDebug::HideThreadFromDebugger(HANDLE hThread)
{
    typedef NTSTATUS(NTAPI * pNtSetInformationThread)(HANDLE, UINT, PVOID, ULONG);
    pNtSetInformationThread NtSetInformationThread = (pNtSetInformationThread)GetProcAddress(GetModuleHandleA("ntdll.dll"), "NtSetInformationThread");

    if (NtSetInformationThread == NULL)
        return false;

    NTSTATUS Status;
    if (hThread == NULL)
        Status = NtSetInformationThread(GetCurrentThread(), 0x11, 0, 0);
    else
        Status = NtSetInformationThread(hThread, 0x11, 0, 0);

    return (Status == 0);
}

bool Debugger::AntiDebug::IsDBK64DriverLoaded()
{
    return Services::IsDriverRunning(this->DBK64Driver);
}

void Debugger::AntiDebug::HideAllThreadsFromDebugger()
{
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE)
    {
        AddLogFmt("Err", "Failed to create thread snapshot in HideAllThreadsFromDebugger");
        return;
    }

    DWORD         pid = GetCurrentProcessId();
    THREADENTRY32 te;
    te.dwSize = sizeof(THREADENTRY32);

    if (Thread32First(hSnapshot, &te))
    {
        do
        {
            if (te.th32OwnerProcessID == pid)
            {
                HANDLE hThread = OpenThread(THREAD_SET_INFORMATION | THREAD_QUERY_INFORMATION, FALSE, te.th32ThreadID);
                if (!hThread)
                {
                    std::stringstream ss;
                    ss << "Failed to open thread for HideAllThreadsFromDebugger: " << GetLastError();
                    AddLogFmt("Warning", ss.str());
                    continue;
                }

                if (!Debugger::AntiDebug::HideThreadFromDebugger(hThread))
                {
                    std::stringstream ss;
                    ss << "Failed to hide thread from debugger: thread id " << te.th32ThreadID;
                    AddLogFmt("Warning", ss.str());
                }

                CloseHandle(hThread);
            }
        } while (Thread32Next(hSnapshot, &te));
    }
    else
    {
        AddLogFmt("Err", "Thread32First failed in HideAllThreadsFromDebugger");
    }

    CloseHandle(hSnapshot);
}
