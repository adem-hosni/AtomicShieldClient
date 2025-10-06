#pragma once
#include "StdInc.h"
#include "KernelCalls.h"

#define USER_SHARED_DATA ((KUSER_SHARED_DATA* const)0x7FFE0000)

enum eDebugDetectionFlags
{
    EXECUTION_ERROR = -1,
    NONE,
    DEBUG_HARDWARE_REGISTERS,
    DEBUG_WINAPI_DEBUGGER,
    DEBUG_VEH_DEBUGGER,
    DEBUG_PEB,
    DEBUG_PROCESS_DEBUG_FLAGS,
    DEBUG_KNOWN_DEBUGGER_PROCESS,
    DEBUG_REMOTE_DEBUGGER,
    DEBUG_KERNEL_DEBUGGER,
    DEBUG_HEAP_FLAG,
    DEBUG_CLOSEHANDLE,
    DEBUG_DEBUG_PORT,
    DEBUG_KNOWN_DEBUGGER_WINDOW
};

class CAntiDebugging
{
public:
    CAntiDebugging(void* (*DetectionCallback)(eDebugDetectionFlags, std::string));

    void StartPulse();
    static void StaticPulse(void* pContext) { ((CAntiDebugging*)pContext)->DoPulse(); }

    void _IsHardwareDebuggerPresent();
    bool PreventWindowsDebuggers();

    eDebugDetectionFlags _IsDebuggerPresent();
    eDebugDetectionFlags _IsDebuggerPresent_HeapFlags();
    eDebugDetectionFlags _IsDebuggerPresent_CloseHandle();
    eDebugDetectionFlags _IsDebuggerPresent_RemoteDebugger();
    eDebugDetectionFlags _IsDebuggerPresent_VEH();
    eDebugDetectionFlags _IsDebuggerPresent_PEB();
    eDebugDetectionFlags _IsDebuggerPresent_DebugPort();
    eDebugDetectionFlags _IsDebuggerPresent_ProcessDebugFlags();
    eDebugDetectionFlags _IsKernelDebuggerPresent();
    eDebugDetectionFlags _IsKernelDebuggerPresent_SharedKData();
    eDebugDetectionFlags _ExitCommonDebuggers(std::string* strReason);     
    eDebugDetectionFlags _ExitCommonDebuggerWindows(std::string* strReason);     

    // call ExitProcess in a remote thread on common debuggers

    void DoPulse();

private:
    void* (*m_DetectionCallback)(eDebugDetectionFlags, std::string);

    std::vector<std::string> m_vCommonDebuggerProcesses = {"ollydbg.exe",
                                                           "x64dbg.exe",
                                                           "x32dbg.exe",
                                                           "idaq.exe",
                                                           "idaq64.exe",
                                                           "idag.exe",
                                                           "idag64.exe",
                                                           "idaw.exe",
                                                           "idaw64.exe",
                                                           "ida64.exe",
                                                           "ImmunityDebugger.exe",
                                                           "Wireshark.exe",
                                                           "Fiddler.exe",
                                                           "HTTPDebuggerUI.exe",
                                                           "HTTPDebuggerSvc.exe",
                                                           "ProcessHacker.exe",
                                                           "ProcessHacker2.exe",
                                                           "ProcessHacker3.exe",
                                                           "procexp.exe",
                                                           "SystemInformer.exe",
                                                           "procexp64.exe",
                                                           "Cheat Engine.exe",
                                                           "CheatEngine-x86_64-SSE4-AVX2.exe",
                                                           "CheatEngine-i386-SSE4-AVX2.exe",
                                                           "CheatEngine-i386-SSE4-AVX2-32bit.exe",
                                                           "dbgview.exe",
                                                           "DebugView64.exe"};
    std::vector<std::string> vCommonDebuggerWindows = {
        "x64dbg",        "x32dbg",         "OllyDbg",          "IDA",       "Immunity Debugger", "Cheat Engine", "Wireshark", "Fiddler",
        "HTTP Debugger", "Process Hacker", "Process Explorer", "DebugView",
           "System Informer"};
};

typedef struct _KSYSTEM_TIME
{
    ULONG LowPart;
    LONG  High1Time;
    LONG  High2Time;
} KSYSTEM_TIME, *PKSYSTEM_TIME;

typedef enum _NT_PRODUCT_TYPE
{
    NtProductWinNt = 1,
    NtProductLanManNt = 2,
    NtProductServer = 3
} NT_PRODUCT_TYPE;

typedef enum _ALTERNATIVE_ARCHITECTURE_TYPE
{
    StandardDesign = 0,
    NEC98x86 = 1,
    EndAlternatives = 2
} ALTERNATIVE_ARCHITECTURE_TYPE;

typedef struct _KUSER_SHARED_DATA            // https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntddk/ns-ntddk-kuser_shared_data
{
    ULONG                         TickCountLowDeprecated;
    ULONG                         TickCountMultiplier;
    KSYSTEM_TIME                  InterruptTime;
    KSYSTEM_TIME                  SystemTime;
    KSYSTEM_TIME                  TimeZoneBias;
    USHORT                        ImageNumberLow;
    USHORT                        ImageNumberHigh;
    WCHAR                         NtSystemRoot[260];
    ULONG                         MaxStackTraceDepth;
    ULONG                         CryptoExponent;
    ULONG                         TimeZoneId;
    ULONG                         LargePageMinimum;
    ULONG                         AitSamplingValue;
    ULONG                         AppCompatFlag;
    ULONGLONG                     RNGSeedVersion;
    ULONG                         GlobalValidationRunlevel;
    LONG                          TimeZoneBiasStamp;
    ULONG                         NtBuildNumber;
    NT_PRODUCT_TYPE               NtProductType;
    BOOLEAN                       ProductTypeIsValid;
    BOOLEAN                       Reserved0[1];
    USHORT                        NativeProcessorArchitecture;
    ULONG                         NtMajorVersion;
    ULONG                         NtMinorVersion;
    BOOLEAN                       ProcessorFeatures[64];
    ULONG                         Reserved1;
    ULONG                         Reserved3;
    ULONG                         TimeSlip;
    ALTERNATIVE_ARCHITECTURE_TYPE AlternativeArchitecture;
    ULONG                         BootId;
    LARGE_INTEGER                 SystemExpirationDate;
    ULONG                         SuiteMask;
    BOOLEAN                       KdDebuggerEnabled;
    union
    {
        UCHAR MitigationPolicies;
        struct
        {
            UCHAR NXSupportPolicy : 2;
            UCHAR SEHValidationPolicy : 2;
            UCHAR CurDirDevicesSkippedForDlls : 2;
            UCHAR Reserved : 2;
        };
    };
    USHORT  CyclesPerYield;
    ULONG   ActiveConsoleId;
    ULONG   DismountCount;
    ULONG   ComPlusPackage;
    ULONG   LastSystemRITEventTickCount;
    ULONG   NumberOfPhysicalPages;
    BOOLEAN SafeBootMode;
    union
    {
        UCHAR VirtualizationFlags;
        struct
        {
            UCHAR ArchStartedInEl2 : 1;
            UCHAR QcSlIsSupported : 1;
        };
    };
    UCHAR Reserved12[2];
    union
    {
        ULONG SharedDataFlags;
        struct
        {
            ULONG DbgErrorPortPresent : 1;
            ULONG DbgElevationEnabled : 1;
            ULONG DbgVirtEnabled : 1;
            ULONG DbgInstallerDetectEnabled : 1;
            ULONG DbgLkgEnabled : 1;
            ULONG DbgDynProcessorEnabled : 1;
            ULONG DbgConsoleBrokerEnabled : 1;
            ULONG DbgSecureBootEnabled : 1;
            ULONG DbgMultiSessionSku : 1;
            ULONG DbgMultiUsersInSessionSku : 1;
            ULONG DbgStateSeparationEnabled : 1;
            ULONG SpareBits : 21;
        } Dbg;
    } DbgUnion;
    ULONG     DataFlagsPad[1];
    ULONGLONG TestRetInstruction;
    LONGLONG  QpcFrequency;
    ULONG     SystemCall;
    ULONG     Reserved2;
    ULONGLONG FullNumberOfPhysicalPages;
    ULONGLONG SystemCallPad[1];
    union
    {
        KSYSTEM_TIME TickCount;
        ULONG64      TickCountQuad;
        struct
        {
            ULONG ReservedTickCountOverlay[3];
            ULONG TickCountPad[1];
        } DUMMYSTRUCTNAME;
    } DUMMYUNIONNAME3;
    ULONG     Cookie;
    ULONG     CookiePad[1];
    LONGLONG  ConsoleSessionForegroundProcessId;
    ULONGLONG TimeUpdateLock;
    ULONGLONG BaselineSystemTimeQpc;
    ULONGLONG BaselineInterruptTimeQpc;
    ULONGLONG QpcSystemTimeIncrement;
    ULONGLONG QpcInterruptTimeIncrement;
    UCHAR     QpcSystemTimeIncrementShift;
    UCHAR     QpcInterruptTimeIncrementShift;
    USHORT    UnparkedProcessorCount;
    ULONG     EnclaveFeatureMask[4];
    ULONG     TelemetryCoverageRound;
    USHORT    UserModeGlobalLogger[16];
    ULONG     ImageFileExecutionOptions;
    ULONG     LangGenerationCount;
    ULONGLONG Reserved4;
    ULONGLONG InterruptTimeBias;
    ULONGLONG QpcBias;
    ULONG     ActiveProcessorCount;
    UCHAR     ActiveGroupCount;
    UCHAR     Reserved9;
    union
    {
        USHORT QpcData;
        struct
        {
            UCHAR QpcBypassEnabled;
            UCHAR QpcReserved;
        };
    };
    LARGE_INTEGER        TimeZoneBiasEffectiveStart;
    LARGE_INTEGER        TimeZoneBiasEffectiveEnd;
    XSTATE_CONFIGURATION XState;
    KSYSTEM_TIME         FeatureConfigurationChangeStamp;
    ULONG                Spare;
    ULONG64              UserPointerAuthMask;
    XSTATE_CONFIGURATION XStateArm64;
    ULONG                Reserved10[210];
} KUSER_SHARED_DATA, *PKUSER_SHARED_DATA;

#ifndef _PROCESS_BASIC_INFORMATION_DEFINED
typedef struct _PROCESS_BASIC_INFORMATION_INTERNAL
{
    PVOID     Reserved1;
    PVOID     PebBaseAddress;
    PVOID     Reserved2[2];
    ULONG_PTR UniqueProcessId;
    PVOID     Reserved3;
} PROCESS_BASIC_INFORMATION_INTERNAL;
#define _PROCESS_BASIC_INFORMATION_DEFINED
#endif

typedef NTSTATUS(NTAPI* NtQueryInformationProcess_t)(HANDLE ProcessHandle, UINT ProcessInformationClass, PVOID ProcessInformation,
                                                     ULONG ProcessInformationLength, PULONG ReturnLength);
