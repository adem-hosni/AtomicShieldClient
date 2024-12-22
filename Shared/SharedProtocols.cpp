#include "SharedProtocols.h"

void SharedProtocols::EnableProcessMitigations(bool useDEP, bool useASLR, bool useDynamicCode, bool useStrictHandles, bool useSystemCallDisable)
{
    if (useDEP)
    {
        PROCESS_MITIGATION_DEP_POLICY depPolicy = {0};            // DEP Policy
        depPolicy.Enable = 1;
        depPolicy.Permanent = 1;

        if (!SetProcessMitigationPolicy(ProcessDEPPolicy, &depPolicy, sizeof(depPolicy)))
        {
            SharedUtil::AddDebugLog("Failed to set DEP policy @ EnableProcessMitigations: %d", GetLastError());
        }
    }

    if (useASLR)
    {
        PROCESS_MITIGATION_ASLR_POLICY aslrPolicy = {0};            // ASLR Policy
        aslrPolicy.EnableBottomUpRandomization = 1;
        aslrPolicy.EnableForceRelocateImages = 1;
        aslrPolicy.EnableHighEntropy = 1;
        aslrPolicy.DisallowStrippedImages = 1;

        if (!SetProcessMitigationPolicy(ProcessASLRPolicy, &aslrPolicy, sizeof(aslrPolicy)))
        {
            SharedUtil::AddDebugLog("Failed to set ASLR policy @ EnableProcessMitigations: %d", GetLastError());
        }
    }

    if (useDynamicCode)
    {
        PROCESS_MITIGATION_DYNAMIC_CODE_POLICY dynamicCodePolicy = {
            0};            // Dynamic Code Policy -> can prevent VirtualProtect calls on .text sections of loaded modules from working
        dynamicCodePolicy.ProhibitDynamicCode = 1;

        if (!SetProcessMitigationPolicy(ProcessDynamicCodePolicy, &dynamicCodePolicy, sizeof(dynamicCodePolicy)))
        {
            SharedUtil::AddDebugLog("Failed to set dynamic code policy @ EnableProcessMitigations: %d", GetLastError());
        }
    }

    if (useStrictHandles)
    {
        PROCESS_MITIGATION_STRICT_HANDLE_CHECK_POLICY handlePolicy = {0};            // Strict Handle Check Policy
        handlePolicy.RaiseExceptionOnInvalidHandleReference = 1;
        handlePolicy.HandleExceptionsPermanentlyEnabled = 1;

        if (!SetProcessMitigationPolicy(ProcessStrictHandleCheckPolicy, &handlePolicy, sizeof(handlePolicy)))
        {
            SharedUtil::AddDebugLog("Failed to set strict handle check policy @ EnableProcessMitigations: %d", GetLastError());
        }
    }

    if (useSystemCallDisable)
    {
        PROCESS_MITIGATION_SYSTEM_CALL_DISABLE_POLICY syscallPolicy = {0};            // System Call Disable Policy
        syscallPolicy.DisallowWin32kSystemCalls = 1;

        if (!SetProcessMitigationPolicy(ProcessSystemCallDisablePolicy, &syscallPolicy, sizeof(syscallPolicy)))
        {
            SharedUtil::AddDebugLog("Failed to set system call disable policy @ EnableProcessMitigations: %d", GetLastError());
        }
    }
}
