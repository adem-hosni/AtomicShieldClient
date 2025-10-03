
#include "cAntiDebugging.hpp"
#include <sstream>
#include <chrono>
#include <SharedUtil.h>


using namespace AntiDebugging;

static void ADLog(const char* level, const std::string& msg)
{
    std::stringstream ss;
    ss << "[" << level << "] " << msg;
    SharedUtil::AddDebugLog(ss.str().c_str());
}

cAntiDebugging& cAntiDebugging::Instance()
{
    static cAntiDebugging instance;
    return instance;
}

cAntiDebugging::cAntiDebugging() : m_thread(nullptr), m_running(false), m_shutdownRequested(false)
{
    // common debugger names kept for future use/extension
    m_commonDebuggerProcesses.push_back(L"x64dbg.exe");
    m_commonDebuggerProcesses.push_back(L"CheatEngine.exe");
    m_commonDebuggerProcesses.push_back(L"idaq64.exe");
    m_commonDebuggerProcesses.push_back(L"kd.exe");
    m_commonDebuggerProcesses.push_back(L"DbgX.Shell.exe");

    // register a few builtin detection functions
    AddDetectionFunction(
        []() -> DetectionFlags
        {
            if (IsDebuggerPresent())
                return DetectionFlags::DEBUG_ISDEBUGGERPRESENT;
            return DetectionFlags::NONE;
        });

    AddDetectionFunction(
        []() -> DetectionFlags
        {
            BOOL remote = FALSE;
            if (CheckRemoteDebuggerPresent(GetCurrentProcess(), &remote) && remote)
                return DetectionFlags::DEBUG_REMOTEDBG;
            return DetectionFlags::NONE;
        });

    // PEB BeingDebugged check
    AddDetectionFunction(
        []() -> DetectionFlags
        {
    // PEB access - using inline assembly/structure free approach
    // On x86/x64, use FS/GS - but we use documented API: NtQueryInformationProcess is a better alternative.
    // Minimal portable approach: use TEB/PEB via Windows internal structures (works on MSVC)
#if defined(_M_X64) || defined(__x86_64__)
            PPEB pPeb = (PPEB)__readgsqword(0x60);
#else
            PPEB pPeb = (PPEB)__readfsdword(0x30);
#endif
            if (pPeb)
            {
                if (pPeb->BeingDebugged)
                    return DetectionFlags::DEBUG_PEB_BEINGDEBUGGED;
            }
            return DetectionFlags::NONE;
        });

    // NtQueryInformationProcess: ProcessDebugPort / ProcessDebugFlags / ProcessDebugObjectHandle checks
    AddDetectionFunction(
        []() -> DetectionFlags
        {
            // We'll try ProcessDebugPort as a lightweight check
            typedef NTSTATUS(NTAPI * pNtQueryInfo)(HANDLE, UINT, PVOID, ULONG, PULONG);
            static pNtQueryInfo NtQueryInformationProcess = nullptr;
            if (!NtQueryInformationProcess)
                NtQueryInformationProcess = (pNtQueryInfo)GetProcAddress(GetModuleHandleA("ntdll.dll"), "NtQueryInformationProcess");
            if (!NtQueryInformationProcess)
                return DetectionFlags::EXECUTION_ERROR;

            ULONG     returnLength = 0;
            ULONG_PTR debugPort = 0;
            NTSTATUS  status = NtQueryInformationProcess(GetCurrentProcess(), 7 /*ProcessDebugPort*/, &debugPort, sizeof(debugPort), &returnLength);
            if (status == 0 && debugPort != 0)
                return DetectionFlags::DEBUG_NT_QUERY_INFO;

            return DetectionFlags::NONE;
        });
}

cAntiDebugging::~cAntiDebugging()
{
    Stop();
}

void cAntiDebugging::Start()
{
    bool expected = false;
    if (!m_running.compare_exchange_strong(expected, true))
    {
        ADLog("Info", "AntiDebug already running (Start skipped)");
        return;
    }

    m_shutdownRequested.store(false);

    m_thread = std::make_unique<std::thread>(&cAntiDebugging::DetectionThreadMain, this);
    ADLog("Info", "Started AntiDebug detection thread");
}

void cAntiDebugging::Stop()
{
    if (!m_running.load())
        return;

    m_shutdownRequested.store(true);

    if (m_thread && m_thread->joinable())
    {
        m_thread->join();
    }
    m_thread.reset();
    m_running.store(false);
    ADLog("Info", "Stopped AntiDebug detection thread");
}

void cAntiDebugging::AddFlagged(DetectionFlags f)
{
    std::lock_guard<std::mutex> l(m_flagsMutex);
    if (f == DetectionFlags::NONE)
        return;
    auto it = std::find(m_detectedFlags.begin(), m_detectedFlags.end(), f);
    if (it == m_detectedFlags.end())
    {
        m_detectedFlags.push_back(f);
        std::stringstream ss;
        ss << "Flag added: " << static_cast<int>(f);
        ADLog("Detection", ss.str());
    }
}

bool cAntiDebugging::IsDBK64Loaded()
{
    // Uses Services::IsDriverRunning if implemented. If not, returns false.
    try
    {
        return Services::IsDriverRunning(m_dbk64);
    }
    catch (...)
    {
        return false;
    }
}

void cAntiDebugging::DetectionThreadMain()
{
    // Attempt windows debugger prevention early
    if (!PreventWindowsDebuggers())
    {
        ADLog("Warning", "PreventWindowsDebuggers failed (may still continue)");
    }

    while (!m_shutdownRequested.load())
    {
        // spawn a thread to check hardware debug registers (it suspends threads)
        try
        {
            std::thread hwThread(&cAntiDebugging::HardwareRegisterScanThread, this);
            hwThread.detach();
        }
        catch (...)
        {
            ADLog("Warning", "Failed to spawn hardware register scan thread");
        }

        // run registered detection functions
        RunDetectionFunctions();

        // DBK64 driver check
        if (IsDBK64Loaded())
        {
            AddFlagged(DetectionFlags::DEBUG_DBK64_DRIVER);
        }

        // loop delay
        for (int i = 0; i < m_loopDelayMs / 50 && !m_shutdownRequested.load(); ++i)
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    ADLog("Info", "Detection thread main exiting");
}

void cAntiDebugging::RunDetectionFunctions()
{
    std::lock_guard<std::mutex> lock(m_detectMutex);
    for (const auto& fn : m_detectionFns)
    {
        try
        {
            DetectionFlags result = fn();
            if (result != DetectionFlags::NONE && result != DetectionFlags::EXECUTION_ERROR)
            {
                AddFlagged(result);
            }
            else if (result == DetectionFlags::EXECUTION_ERROR)
            {
                ADLog("Err", "A detection function returned EXECUTION_ERROR");
            }
        }
        catch (...)
        {
            ADLog("Err", "Exception thrown executing a detection function");
        }
    }
}

/*
   HardwareRegisterScanThread
   Suspends other threads (of the same process) one by one, gets CONTEXT with CONTEXT_DEBUG_REGISTERS
   and checks Dr0..Dr7 for non-zero values.
*/
void cAntiDebugging::HardwareRegisterScanThread(void* ctx)
{
    if (!ctx)
        return;
    cAntiDebugging* self = reinterpret_cast<cAntiDebugging*>(ctx);

    THREADENTRY32 te32;
    te32.dwSize = sizeof(THREADENTRY32);

    HANDLE hThreadSnap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (hThreadSnap == INVALID_HANDLE_VALUE)
    {
        ADLog("Err", "CreateToolhelp32Snapshot failed in HardwareRegisterScanThread");
        return;
    }

    DWORD currentPID = GetCurrentProcessId();
    DWORD currentTID = GetCurrentThreadId();

    if (!Thread32First(hThreadSnap, &te32))
    {
        CloseHandle(hThreadSnap);
        ADLog("Err", "Thread32First failed in HardwareRegisterScanThread");
        return;
    }

    do
    {
        if (te32.th32OwnerProcessID == currentPID && te32.th32ThreadID != currentTID)
        {
            HANDLE hThread = OpenThread(THREAD_GET_CONTEXT | THREAD_SUSPEND_RESUME | THREAD_QUERY_INFORMATION, FALSE, te32.th32ThreadID);
            if (!hThread)
            {
                std::stringstream ss;
                ss << "OpenThread failed for tid " << te32.th32ThreadID << " err=" << GetLastError();
                ADLog("Warning", ss.str());
                continue;
            }

            if (SuspendThread(hThread) == (DWORD)-1)
            {
                std::stringstream ss;
                ss << "SuspendThread failed for tid " << te32.th32ThreadID << " err=" << GetLastError();
                ADLog("Warning", ss.str());
                CloseHandle(hThread);
                continue;
            }

            CONTEXT ctxC = {0};
            ctxC.ContextFlags = CONTEXT_DEBUG_REGISTERS;

            if (GetThreadContext(hThread, &ctxC))
            {
                if (ctxC.Dr0 || ctxC.Dr1 || ctxC.Dr2 || ctxC.Dr3 || ctxC.Dr6 || ctxC.Dr7)
                {
                    ADLog("Detection", "Hardware debug register(s) detected");
                    self->AddFlagged(DetectionFlags::DEBUG_HW_REGISTERS);
                    ResumeThread(hThread);
                    CloseHandle(hThread);
                    CloseHandle(hThreadSnap);
                    return;
                }
            }
            else
            {
                std::stringstream ss;
                ss << "GetThreadContext failed for tid " << te32.th32ThreadID << " err=" << GetLastError();
                ADLog("Warning", ss.str());
            }

            ResumeThread(hThread);
            CloseHandle(hThread);
        }
    } while (Thread32Next(hThreadSnap, &te32));

    CloseHandle(hThreadSnap);
}

/*
   PreventWindowsDebuggers - attempt to patch ntdll's DbgBreakPoint and DbgUiRemoteBreakin by writing 0xC3 (ret)
   This is an intrusive technique; it may fail under some system protections.
*/
bool cAntiDebugging::PreventWindowsDebuggers()
{
    HMODULE ntdll = GetModuleHandleA("ntdll.dll");
    if (!ntdll)
    {
        ADLog("Err", "GetModuleHandleA(ntdll.dll) failed in PreventWindowsDebuggers");
        return false;
    }

    DWORD   oldProt = 0;
    FARPROC addrDbgBreak = GetProcAddress(ntdll, "DbgBreakPoint");
    FARPROC addrDbgUi = GetProcAddress(ntdll, "DbgUiRemoteBreakin");

    if (addrDbgBreak)
    {
        if (VirtualProtect(addrDbgBreak, 1, PAGE_EXECUTE_READWRITE, &oldProt))
        {
            __try
            {
                *(BYTE*)addrDbgBreak = 0xC3;            // ret
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                ADLog("Err", "Exception writing to DbgBreakPoint");
                VirtualProtect(addrDbgBreak, 1, oldProt, &oldProt);
                return false;
            }
            VirtualProtect(addrDbgBreak, 1, oldProt, &oldProt);
        }
    }

    if (addrDbgUi)
    {
        if (VirtualProtect(addrDbgUi, 1, PAGE_EXECUTE_READWRITE, &oldProt))
        {
            __try
            {
                *(BYTE*)addrDbgUi = 0xC3;            // ret
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                ADLog("Err", "Exception writing to DbgUiRemoteBreakin");
                VirtualProtect(addrDbgUi, 1, oldProt, &oldProt);
                return false;
            }
            VirtualProtect(addrDbgUi, 1, oldProt, &oldProt);
        }
    }

    return true;
}

/*
   HideThreadFromDebugger - calls NtSetInformationThread(ThreadHideFromDebugger) (0x11)
*/
bool cAntiDebugging::HideThreadFromDebugger(HANDLE hThread)
{
    typedef NTSTATUS(NTAPI * pNtSetInfo)(HANDLE, ULONG, PVOID, ULONG);
    static pNtSetInfo NtSetInformationThread = (pNtSetInfo)GetProcAddress(GetModuleHandleA("ntdll.dll"), "NtSetInformationThread");
    if (!NtSetInformationThread)
        return false;

    NTSTATUS status;
    if (hThread == NULL)
        status = NtSetInformationThread(GetCurrentThread(), 0x11 /*ThreadHideFromDebugger*/, nullptr, 0);
    else
        status = NtSetInformationThread(hThread, 0x11, nullptr, 0);

    return (status == 0);
}

/*
   HideAllThreadsFromDebugger - enumerate process threads and call HideThreadFromDebugger on each
*/
void cAntiDebugging::HideAllThreadsFromDebugger()
{
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE)
    {
        ADLog("Err", "CreateToolhelp32Snapshot failed in HideAllThreadsFromDebugger");
        return;
    }

    THREADENTRY32 te = {0};
    te.dwSize = sizeof(te);
    DWORD pid = GetCurrentProcessId();

    if (!Thread32First(hSnapshot, &te))
    {
        CloseHandle(hSnapshot);
        ADLog("Err", "Thread32First failed in HideAllThreadsFromDebugger");
        return;
    }

    do
    {
        if (te.th32OwnerProcessID == pid)
        {
            HANDLE hThread = OpenThread(THREAD_SET_INFORMATION | THREAD_QUERY_INFORMATION, FALSE, te.th32ThreadID);
            if (!hThread)
            {
                std::stringstream ss;
                ss << "OpenThread failed in HideAllThreadsFromDebugger tid=" << te.th32ThreadID << " err=" << GetLastError();
                ADLog("Warning", ss.str());
                continue;
            }

            if (!HideThreadFromDebugger(hThread))
            {
                std::stringstream ss;
                ss << "HideThreadFromDebugger failed for tid=" << te.th32ThreadID << " err=" << GetLastError();
                ADLog("Warning", ss.str());
            }

            CloseHandle(hThread);
        }
    } while (Thread32Next(hSnapshot, &te));

    CloseHandle(hSnapshot);
}
