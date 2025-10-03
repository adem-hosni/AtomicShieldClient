#pragma once

#include <windows.h>
#include <tlhelp32.h>
#include <vector>
#include <list>
#include <string>
#include <functional>
#include <mutex>
#include <memory>
#include <thread>
#include <atomic>



namespace Services
{
    // optional - used to check DBK64 driver presence (if you have this, fine; otherwise its implementation below will simply return false)
    bool IsDriverRunning(const std::wstring& driverName);
}            

namespace AntiDebugging
{
    enum class DetectionFlags : int
    {
        NONE = 0,
        DEBUG_HW_REGISTERS = 1,
        DEBUG_DBK64_DRIVER = 2,
        DEBUG_ISDEBUGGERPRESENT = 3,
        DEBUG_REMOTEDBG = 4,
        DEBUG_PEB_BEINGDEBUGGED = 5,
        DEBUG_NT_QUERY_INFO = 6,
        // negative value reserved for execution error
        EXECUTION_ERROR = -1
    };

    class cAntiDebugging
    {
    public:
        static cAntiDebugging& Instance();

        cAntiDebugging();
        ~cAntiDebugging();

        cAntiDebugging(const cAntiDebugging&) = delete;
        cAntiDebugging& operator=(const cAntiDebugging&) = delete;

        void Start();            // start detection thread (no-op if already started)
        void Stop();             // signal shutdown and join thread

        bool IsRunning() const { return m_running.load(); }

        // detection function registration. Each function should return a DetectionFlags value (NONE if no detection)
        template <typename Func>
        void AddDetectionFunction(Func fn)
        {
            std::lock_guard<std::mutex> lock(m_detectMutex);
            m_detectionFns.emplace_back(fn);
        }

        // helper utilities
        static bool PreventWindowsDebuggers();                         // attempts to patch common Dbg* routines
        static bool HideThreadFromDebugger(HANDLE hThread);            // uses NtSetInformationThread
        static void HideAllThreadsFromDebugger();

    private:
        void        DetectionThreadMain();
        static void HardwareRegisterScanThread(void* ctx);            // suspends other threads and checks debug regs
        void        RunDetectionFunctions();                          // iterate detection functions and log/add flags

        std::unique_ptr<std::thread>                 m_thread;
        std::atomic<bool>                            m_running;
        std::atomic<bool>                            m_shutdownRequested;
        std::mutex                                   m_detectMutex;
        std::vector<std::function<DetectionFlags()>> m_detectionFns;
        std::list<DetectionFlags>                    m_detectedFlags;            // keep unique flags
        std::mutex                                   m_flagsMutex;

        const std::wstring      m_dbk64 = L"DBK64.sys";
        std::list<std::wstring> m_commonDebuggerProcesses;
        int                     m_loopDelayMs = 1000;

        void AddFlagged(DetectionFlags f);
        bool IsDBK64Loaded();
    };

}            // namespace AntiDebugging
