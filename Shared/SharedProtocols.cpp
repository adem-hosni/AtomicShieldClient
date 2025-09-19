#include "SharedProtocols.h"

void SharedProtocols::EnableProcessMitigations()
{
    PROCESS_MITIGATION_DEP_POLICY depPolicy = {0};            // DEP Policy
    depPolicy.Enable = 1;
    depPolicy.Permanent = 1;

    if (!SetProcessMitigationPolicy(ProcessDEPPolicy, &depPolicy, sizeof(depPolicy)))
    {
        SharedUtil::AddDebugLog("Failed to set DEP policy @ EnableProcessMitigations: 0x%x", GetLastError());
    }

    PROCESS_MITIGATION_ASLR_POLICY aslrPolicy = {0};            // ASLR Policy
    aslrPolicy.EnableBottomUpRandomization = 1;
    aslrPolicy.EnableForceRelocateImages = 1;
    aslrPolicy.EnableHighEntropy = 1;
    aslrPolicy.DisallowStrippedImages = 1;

    if (!SetProcessMitigationPolicy(ProcessASLRPolicy, &aslrPolicy, sizeof(aslrPolicy)))
    {
        SharedUtil::AddDebugLog("Failed to set ASLR policy @EnableProcessMitigations: 0x%x", GetLastError());
    }

    PROCESS_MITIGATION_DYNAMIC_CODE_POLICY dynamicCodePolicy = {
        0};            // Dynamic Code Policy -> can prevent VirtualProtect calls on .text sections of loaded modules from working
    dynamicCodePolicy.ProhibitDynamicCode = 1;

    if (!SetProcessMitigationPolicy(ProcessDynamicCodePolicy, &dynamicCodePolicy, sizeof(dynamicCodePolicy)))
    {
        SharedUtil::AddDebugLog("Failed to set dynamic code policy @ EnableProcessMitigations: 0x%x", GetLastError());
    }

    PROCESS_MITIGATION_STRICT_HANDLE_CHECK_POLICY handlePolicy = {0};            // Strict Handle Check Policy
    handlePolicy.RaiseExceptionOnInvalidHandleReference = 1;
    handlePolicy.HandleExceptionsPermanentlyEnabled = 1;

    if (!SetProcessMitigationPolicy(ProcessStrictHandleCheckPolicy, &handlePolicy, sizeof(handlePolicy)))
    {
        SharedUtil::AddDebugLog("Failed to set strict handle check policy @EnableProcessMitigations: 0x%x", GetLastError());
    }

    PROCESS_MITIGATION_SYSTEM_CALL_DISABLE_POLICY syscallPolicy = {0};            // System Call Disable Policy
    syscallPolicy.DisallowWin32kSystemCalls = 1;

    if (!SetProcessMitigationPolicy(ProcessSystemCallDisablePolicy, &syscallPolicy, sizeof(syscallPolicy)))
    {
        SharedUtil::AddDebugLog("Failed to set system call disable policy @EnableProcessMitigations: 0x%x", GetLastError());
    }

    PROCESS_MITIGATION_BINARY_SIGNATURE_POLICY sp = {};
    sp.MicrosoftSignedOnly = 1;
    if (!SetProcessMitigationPolicy(ProcessSignaturePolicy, &sp, sizeof(sp)))
    {
        SharedUtil::AddDebugLog("Failed to set process signature policy @EnableProcessMitigations: 0x%x", GetLastError());
    }
}

void SharedProtocols::DisableProcessMitigations()
{
    PROCESS_MITIGATION_DYNAMIC_CODE_POLICY dynamicCodePolicy = {
        0};            // Dynamic Code Policy
    dynamicCodePolicy.ProhibitDynamicCode = 0;
    if (!SetProcessMitigationPolicy(ProcessDynamicCodePolicy, &dynamicCodePolicy, sizeof(dynamicCodePolicy)))
    {
        SharedUtil::AddDebugLog("Failed to disable dynamic code policy @ DisableProcessMitigations: 0x%x", GetLastError());
    }

    PROCESS_MITIGATION_STRICT_HANDLE_CHECK_POLICY handlePolicy = {0};            // Strict Handle Check Policy
    handlePolicy.RaiseExceptionOnInvalidHandleReference = 0;
    handlePolicy.HandleExceptionsPermanentlyEnabled = 0;
    if (!SetProcessMitigationPolicy(ProcessStrictHandleCheckPolicy, &handlePolicy, sizeof(handlePolicy)))
    {
        SharedUtil::AddDebugLog("Failed to disable strict handle check policy @DisableProcessMitigations: 0x%x", GetLastError());
    }

    PROCESS_MITIGATION_SYSTEM_CALL_DISABLE_POLICY syscallPolicy = {0};            // System Call Disable Policy
    syscallPolicy.DisallowWin32kSystemCalls = 0;
    if (!SetProcessMitigationPolicy(ProcessSystemCallDisablePolicy, &syscallPolicy, sizeof(syscallPolicy)))
    {
        SharedUtil::AddDebugLog("Failed to disable system call disable policy @DisableProcessMitigations: 0x%x", GetLastError());
    }

    PROCESS_MITIGATION_BINARY_SIGNATURE_POLICY sp = {};
    sp.MicrosoftSignedOnly = 0;
    if (!SetProcessMitigationPolicy(ProcessSignaturePolicy, &sp, sizeof(sp)))
    {
        SharedUtil::AddDebugLog("Failed to disable process signature policy @DisableProcessMitigations: 0x%x", GetLastError());
    }

    PROCESS_MITIGATION_DEP_POLICY depPolicy = {0};            // DEP Policy
    depPolicy.Enable = 0;
    depPolicy.Permanent = 0;
    if (!SetProcessMitigationPolicy(ProcessDEPPolicy, &depPolicy, sizeof(depPolicy)))
    {
        SharedUtil::AddDebugLog("Failed to disable DEP policy @ DisableProcessMitigations: 0x%x", GetLastError());
    }
}

void SharedProtocols::CheckLauncherProcess()
{
    std::string strLauncherName = SharedUtil::GetParentProcessName();
    strLauncherName = strLauncherName.substr(strLauncherName.length() - 12, strLauncherName.length());
#ifndef _DEBUG
    if (strLauncherName != "explorer.exe" || strLauncherName != "cmd.exe")
        __fastfail(0);
#endif
}