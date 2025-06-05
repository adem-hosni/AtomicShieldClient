#pragma once
#include <Windows.h>
#include <stdio.h>

#ifndef _NTDEF_
typedef _Return_type_success_(return >= 0) LONG NTSTATUS;
typedef NTSTATUS* PNTSTATUS;
#endif

typedef struct KernelCalls_Sys_SYSCALL_ENTRY
{
    DWORD Hash;
    DWORD Address;
    PVOID SyscallAddress;
} Sys_SYSCALL_ENTRY, *PSys_SYSCALL_ENTRY;

typedef struct KernelCalls_Sys_SYSCALL_LIST
{
    DWORD             Count;
    Sys_SYSCALL_ENTRY Entries[600];
} Sys_SYSCALL_LIST, *PSys_SYSCALL_LIST;

typedef struct KernelCalls_Sys_PEB_LDR_DATA
{
    BYTE       Reserved1[8];
    PVOID      Reserved2[3];
    LIST_ENTRY InMemoryOrderModuleList;
} Sys_PEB_LDR_DATA, *PSys_PEB_LDR_DATA;

typedef struct KernelCalls_Sys_LDR_DATA_TABLE_ENTRY
{
    PVOID      Reserved1[2];
    LIST_ENTRY InMemoryOrderLinks;
    PVOID      Reserved2[2];
    PVOID      DllBase;
} Sys_LDR_DATA_TABLE_ENTRY, *PSys_LDR_DATA_TABLE_ENTRY;

typedef struct KernelCalls_Sys_PEB
{
    BYTE              Reserved1[2];
    BYTE              BeingDebugged;
    BYTE              Reserved2[1];
    PVOID             Reserved3[2];
    PSys_PEB_LDR_DATA Ldr;
} Sys_PEB, *PSys_PEB;

typedef struct KernelCalls_UNICODE_STRING
{
    USHORT Length;
    USHORT MaximumLength;
    PWSTR  Buffer;
} KernelCalls_UNICODE_STRING, *KernelCalls_PUNICODE_STRING;

typedef enum _KernelCalls_PROCESSINFOCLASS
{
    KernelCalls_ProcessBasicInformation,                      // q: PROCESS_BASIC_INFORMATION, PROCESS_EXTENDED_BASIC_INFORMATION
    KernelCalls_ProcessQuotaLimits,                           // qs: QUOTA_LIMITS, QUOTA_LIMITS_EX
    KernelCalls_ProcessIoCounters,                            // q: IO_COUNTERS
    KernelCalls_ProcessVmCounters,                            // q: VM_COUNTERS, VM_COUNTERS_EX, VM_COUNTERS_EX2
    KernelCalls_ProcessTimes,                                 // q: KERNEL_USER_TIMES
    KernelCalls_ProcessBasePriority,                          // s: KPRIORITY
    KernelCalls_ProcessRaisePriority,                         // s: ULONG
    KernelCalls_ProcessDebugPort,                             // q: HANDLE
    KernelCalls_ProcessExceptionPort,                         // s: PROCESS_EXCEPTION_PORT (requires SeTcbPrivilege)
    KernelCalls_ProcessAccessToken,                           // s: PROCESS_ACCESS_TOKEN
    KernelCalls_ProcessLdtInformation,                        // qs: PROCESS_LDT_INFORMATION // 10
    KernelCalls_ProcessLdtSize,                               // s: PROCESS_LDT_SIZE
    KernelCalls_ProcessDefaultHardErrorMode,                  // qs: ULONG
    KernelCalls_ProcessIoPortHandlers,                        // (kernel-mode only) // PROCESS_IO_PORT_HANDLER_INFORMATION
    KernelCalls_ProcessPooledUsageAndLimits,                  // q: POOLED_USAGE_AND_LIMITS
    KernelCalls_ProcessWorkingSetWatch,                       // q: PROCESS_WS_WATCH_INFORMATION[]; s: void
    KernelCalls_ProcessUserModeIOPL,                          // qs: ULONG (requires SeTcbPrivilege)
    KernelCalls_ProcessEnableAlignmentFaultFixup,             // s: BOOLEAN
    KernelCalls_ProcessPriorityClass,                         // qs: PROCESS_PRIORITY_CLASS
    KernelCalls_ProcessWx86Information,                       // qs: ULONG (requires SeTcbPrivilege) (VdmAllowed)
    KernelCalls_ProcessHandleCount,                           // q: ULONG, PROCESS_HANDLE_INFORMATION // 20
    KernelCalls_ProcessAffinityMask,                          // (q >WIN7)s: KAFFINITY, qs: GROUP_AFFINITY
    KernelCalls_ProcessPriorityBoost,                         // qs: ULONG
    KernelCalls_ProcessDeviceMap,                             // qs: PROCESS_DEVICEMAP_INFORMATION, PROCESS_DEVICEMAP_INFORMATION_EX
    KernelCalls_ProcessSessionInformation,                    // q: PROCESS_SESSION_INFORMATION
    KernelCalls_ProcessForegroundInformation,                 // s: PROCESS_FOREGROUND_BACKGROUND
    KernelCalls_ProcessWow64Information,                      // q: ULONG_PTR
    KernelCalls_ProcessImageFileName,                         // q: UNICODE_STRING
    KernelCalls_ProcessLUIDDeviceMapsEnabled,                 // q: ULONG
    KernelCalls_ProcessBreakOnTermination,                    // qs: ULONG
    KernelCalls_ProcessDebugObjectHandle,                     // q: HANDLE // 30
    KernelCalls_ProcessDebugFlags,                            // qs: ULONG
    KernelCalls_ProcessHandleTracing,                         // q: PROCESS_HANDLE_TRACING_QUERY; s: size 0 disables, otherwise enables
    KernelCalls_ProcessIoPriority,                            // qs: IO_PRIORITY_HINT
    KernelCalls_ProcessExecuteFlags,                          // qs: ULONG
    KernelCalls_ProcessTlsInformation,                        // PROCESS_TLS_INFORMATION // ProcessResourceManagement
    KernelCalls_ProcessCookie,                                // q: ULONG
    KernelCalls_ProcessImageInformation,                      // q: SECTION_IMAGE_INFORMATION
    KernelCalls_ProcessCycleTime,                             // q: PROCESS_CYCLE_TIME_INFORMATION // since VISTA
    KernelCalls_ProcessPagePriority,                          // qs: PAGE_PRIORITY_INFORMATION
    KernelCalls_ProcessInstrumentationCallback,               // s: PVOID or PROCESS_INSTRUMENTATION_CALLBACK_INFORMATION // 40
    KernelCalls_ProcessThreadStackAllocation,                 // s: PROCESS_STACK_ALLOCATION_INFORMATION, PROCESS_STACK_ALLOCATION_INFORMATION_EX
    KernelCalls_ProcessWorkingSetWatchEx,                     // q: PROCESS_WS_WATCH_INFORMATION_EX[]
    KernelCalls_ProcessImageFileNameWin32,                    // q: UNICODE_STRING
    KernelCalls_ProcessImageFileMapping,                      // q: HANDLE (input)
    KernelCalls_ProcessAffinityUpdateMode,                    // qs: PROCESS_AFFINITY_UPDATE_MODE
    KernelCalls_ProcessMemoryAllocationMode,                  // qs: PROCESS_MEMORY_ALLOCATION_MODE
    KernelCalls_ProcessGroupInformation,                      // q: USHORT[]
    KernelCalls_ProcessTokenVirtualizationEnabled,            // s: ULONG
    KernelCalls_ProcessConsoleHostProcess,                    // q: ULONG_PTR // ProcessOwnerInformation
    KernelCalls_ProcessWindowInformation,                     // q: PROCESS_WINDOW_INFORMATION // 50
    KernelCalls_ProcessHandleInformation,                     // q: PROCESS_HANDLE_SNAPSHOT_INFORMATION // since WIN8
    KernelCalls_ProcessMitigationPolicy,                      // s: PROCESS_MITIGATION_POLICY_INFORMATION
    KernelCalls_ProcessDynamicFunctionTableInformation,
    KernelCalls_ProcessHandleCheckingMode,                   // qs: ULONG; s: 0 disables, otherwise enables
    KernelCalls_ProcessSysepAliveCount,                      // q: PROCESS_KEEPALIVE_COUNT_INFORMATION
    KernelCalls_ProcessRevokeFileHandles,                    // s: PROCESS_REVOKE_FILE_HANDLES_INFORMATION
    KernelCalls_ProcessWorkingSetControl,                    // s: PROCESS_WORKING_SET_CONTROL
    KernelCalls_ProcessHandleTable,                          // q: ULONG[] // since WINBLUE
    KernelCalls_ProcessCheckStackExtentsMode,                // qs: ULONG // KPROCESS->CheckStackExtents (CFG)
    KernelCalls_ProcessCommandLineInformation,               // q: UNICODE_STRING // 60
    KernelCalls_ProcessProtectionInformation,                // q: PS_PROTECTION
    KernelCalls_ProcessMemoryExhaustion,                     // PROCESS_MEMORY_EXHAUSTION_INFO // since THRESHOLD
    KernelCalls_ProcessFaultInformation,                     // PROCESS_FAULT_INFORMATION
    KernelCalls_ProcessTelemetryIdInformation,               // q: PROCESS_TELEMETRY_ID_INFORMATION
    KernelCalls_ProcessCommitReleaseInformation,             // PROCESS_COMMIT_RELEASE_INFORMATION
    KernelCalls_ProcessDefaultCpuSetsInformation,            // SYSTEM_CPU_SET_INFORMATION[5]
    KernelCalls_ProcessAllowedCpuSetsInformation,            // SYSTEM_CPU_SET_INFORMATION[5]
    KernelCalls_ProcessSubsystemProcess,
    KernelCalls_ProcessJobMemoryInformation,                            // q: PROCESS_JOB_MEMORY_INFO
    KernelCalls_ProcessInPrivate,                                       // s: void // ETW // since THRESHOLD2 // 70
    KernelCalls_ProcessRaiseUMExceptionOnInvalidHandleClose,            // qs: ULONG; s: 0 disables, otherwise enables
    KernelCalls_ProcessIumChallengeResponse,
    KernelCalls_ProcessChildProcessInformation,                    // q: PROCESS_CHILD_PROCESS_INFORMATION
    KernelCalls_ProcessHighGraphicsPriorityInformation,            // qs: BOOLEAN (requires SeTcbPrivilege)
    KernelCalls_ProcessSubsystemInformation,                       // q: SUBSYSTEM_INFORMATION_TYPE // since REDSTONE2
    KernelCalls_ProcessEnergyValues,                               // q: PROCESS_ENERGY_VALUES, PROCESS_EXTENDED_ENERGY_VALUES
    KernelCalls_ProcessPowerThrottlingState,                       // qs: POWER_THROTTLING_PROCESS_STATE
    KernelCalls_ProcessReserved3Information,                       // ProcessActivityThrottlePolicy // PROCESS_ACTIVITY_THROTTLE_POLICY
    KernelCalls_ProcessWin32kSyscallFilterInformation,             // q: WIN32K_SYSCALL_FILTER
    KernelCalls_ProcessDisableSystemAllowedCpuSets,                // 80
    KernelCalls_ProcessWakeInformation,                            // PROCESS_WAKE_INFORMATION
    KernelCalls_ProcessEnergyTrackingState,                        // PROCESS_ENERGY_TRACKING_STATE
    KernelCalls_ProcessManageWritesToExecutableMemory,             // MANAGE_WRITES_TO_EXECUTABLE_MEMORY // since REDSTONE3
    KernelCalls_ProcessCaptureTrustletLiveDump,
    KernelCalls_ProcessTelemetryCoverage,
    KernelCalls_ProcessEnclaveInformation,
    KernelCalls_ProcessEnableReadWriteVmLogging,                      // PROCESS_READWRITEVM_LOGGING_INFORMATION
    KernelCalls_ProcessUptimeInformation,                             // q: PROCESS_UPTIME_INFORMATION
    KernelCalls_ProcessImageSection,                                  // q: HANDLE
    KernelCalls_ProcessDebugAuthInformation,                          // since REDSTONE4 // 90
    KernelCalls_ProcessSystemResourceManagement,                      // PROCESS_SYSTEM_RESOURCE_MANAGEMENT
    KernelCalls_ProcessSequenceNumber,                                // q: ULONGLONG
    KernelCalls_ProcessLoaderDetour,                                  // since REDSTONE5
    KernelCalls_ProcessSecurityDomainInformation,                     // PROCESS_SECURITY_DOMAIN_INFORMATION
    KernelCalls_ProcessCombineSecurityDomainsInformation,             // PROCESS_COMBINE_SECURITY_DOMAINS_INFORMATION
    KernelCalls_ProcessEnableLogging,                                 // PROCESS_LOGGING_INFORMATION
    KernelCalls_ProcessLeapSecondInformation,                         // PROCESS_LEAP_SECOND_INFORMATION
    KernelCalls_ProcessFiberShadowStackAllocation,                    // PROCESS_FIBER_SHADOW_STACK_ALLOCATION_INFORMATION // since 19H1
    KernelCalls_ProcessFreeFiberShadowStackAllocation,                // PROCESS_FREE_FIBER_SHADOW_STACK_ALLOCATION_INFORMATION
    KernelCalls_ProcessAltSystemCallInformation,                      // qs: BOOLEAN (kernel-mode only) // INT2E // since 20H1 // 100
    KernelCalls_ProcessDynamicEHContinuationTargets,                  // PROCESS_DYNAMIC_EH_CONTINUATION_TARGETS_INFORMATION
    KernelCalls_ProcessDynamicEnforcedCetCompatibleRanges,            // PROCESS_DYNAMIC_ENFORCED_ADDRESS_RANGE_INFORMATION // since 20H2
    KernelCalls_ProcessCreateStateChange,                             // since WIN11
    KernelCalls_ProcessApplyStateChange,
    KernelCalls_ProcessEnableOptionalXStateFeatures,
    KernelCalls_ProcessAltPrefetchParam,            // since 22H1
    KernelCalls_ProcessAssignCpuPartitions,
    KernelCalls_ProcessPriorityClassEx,            // s: PROCESS_PRIORITY_CLASS_EX
    KernelCalls_ProcessMembershipInformation,
    KernelCalls_ProcessEffectiveIoPriority,              // q: IO_PRIORITY_HINT
    KernelCalls_ProcessEffectivePagePriority,            // q: ULONG
    KernelCalls_MaxProcessInfoClass
} KernelCalls_PROCESSINFOCLASS,
    *KernelCalls_PPROCESSINFOCLASS;

typedef enum _KernelCalls_MEMORY_INFORMATION_CLASS
{
    MemoryBasicInformation,
    MemoryWorkingSetInformation,
    MemoryMappedFilenameInformation,
    MemoryRegionInformation,
    MemoryWorkingSetExInformation,
    MemorySharedCommitInformation,
    MemoryImageInformation,
    MemoryRegionInformationEx,
    MemoryPrivilegedBasicInformation,
    MemoryEnclaveImageInformation,
    MemoryBasicInformationCapped
} KernelCalls_MEMORY_INFORMATION_CLASS,
    *KernelCalls_PMEMORY_INFORMATION_CLASS;

typedef struct _KernelCalls_MEMORY_SECTION_NAME
{
    KernelCalls_UNICODE_STRING SectionFileName;
} KernelCalls_MEMORY_SECTION_NAME, *KernelCalls_PMEMORY_SECTION_NAME;

typedef struct KernelCalls_CLIENT_ID
{
    HANDLE UniqueProcess;
    HANDLE UniqueThread;
} KernelCalls_CLIENT_ID, *KernelCalls_PCLIENT_ID;

typedef struct KernelCalls_OBJECT_ATTRIBUTES
{
    ULONG                       Length;
    HANDLE                      RootDirectory;
    KernelCalls_PUNICODE_STRING ObjectName;
    ULONG                       Attributes;
    PVOID                       SecurityDescriptor;
    PVOID                       SecurityQualityOfService;
} KernelCalls_OBJECT_ATTRIBUTES, *KernelCalls_POBJECT_ATTRIBUTES;

EXTERN_C NTSTATUS SysNtReadVirtualMemory(IN HANDLE ProcessHandle, IN PVOID BaseAddress OPTIONAL, OUT PVOID Buffer, IN SIZE_T BufferSize,
                                         OUT PSIZE_T NumberOfBytesRead OPTIONAL);

EXTERN_C NTSTATUS SysNtAllocateVirtualMemory(IN HANDLE ProcessHandle, IN OUT PVOID* BaseAddress, IN ULONG ZeroBits, IN OUT PSIZE_T RegionSize,
                                             IN ULONG AllocationType, IN ULONG Protect);

EXTERN_C NTSTATUS SysNtFreeVirtualMemory(IN HANDLE ProcessHandle, IN OUT PVOID* BaseAddress, IN OUT PSIZE_T RegionSize, IN ULONG FreeType);

EXTERN_C NTSTATUS SysNtQueryVirtualMemory(IN HANDLE ProcessHandle, IN PVOID BaseAddress, IN KernelCalls_MEMORY_INFORMATION_CLASS MemoryInformationClass,
                                          OUT PVOID MemoryInformation, IN SIZE_T MemoryInformationLength, OUT PSIZE_T ReturnLength OPTIONAL);

EXTERN_C NTSTATUS SysNtClose(IN HANDLE Handle);

EXTERN_C NTSTATUS SysNtOpenProcess(OUT PHANDLE ProcessHandle, IN ACCESS_MASK DesiredAccess, IN KernelCalls_POBJECT_ATTRIBUTES ObjectAttributes,
                                   IN KernelCalls_PCLIENT_ID ClientId OPTIONAL);

EXTERN_C NTSTATUS SysNtQueryInformationProcess(IN HANDLE ProcessHandle, IN KernelCalls_PROCESSINFOCLASS ProcessInformationClass, OUT PVOID ProcessInformation,
                                               IN ULONG ProcessInformationLength, OUT PULONG ReturnLength OPTIONAL);