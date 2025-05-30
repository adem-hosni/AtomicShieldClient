#include "StdInc.h"
#include <winhttp.h>

void CCrashHandler::Initialize()
{
    SharedUtil::AddDebugLog("Initializing Crash Handler...");
    SetUnhandledExceptionFilter(SEHTranslator);
}

LONG CCrashHandler::SEHTranslator(EXCEPTION_POINTERS* pException)
{
    if (!pException || !pException->ExceptionRecord || !pException->ContextRecord)
        return EXCEPTION_EXECUTE_HANDLER;

    SharedUtil::AddDebugLog(
        "SEH Exception Caught! Address: 0x%p | Code: 0x%08X\n"
        "\tRAX: 0x%p \tRCX: 0x%p \tRIP: 0x%p",
        pException->ExceptionRecord->ExceptionAddress, pException->ExceptionRecord->ExceptionCode, (void*)pException->ContextRecord->Rax,
        (void*)pException->ContextRecord->Rcx, (void*)pException->ContextRecord->Rip);

    if (!UploadCrashReport(pException))
        SharedUtil::AddDebugLog("Failed to upload crash report!");

    return EXCEPTION_EXECUTE_HANDLER;
}

bool CCrashHandler::UploadCrashReport(EXCEPTION_POINTERS* pException)
{
    jsoncons::json CrashReport = GenerateCrashReport(pException);
    std::string    strCrashReport = CrashReport.to_string();
    DWORD          dwDataSize = static_cast<DWORD>(strCrashReport.size());

    SharedUtil::AddDebugLog("[CrashReport] Crash Report Generated Successfuly! (%d bytes)", dwDataSize);

    // Open session
    HINTERNET hSession = WinHttpOpen(L"AtomicShield/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession)
    {
        SharedUtil::AddDebugLog("WinHttpOpen failed: %lu", GetLastError());
        return false;
    }

    // HTTPS port 443
    HINTERNET hConnect = WinHttpConnect(hSession, L"atomic-shield.com", INTERNET_DEFAULT_HTTPS_PORT, 0);
    if (!hConnect)
    {
        SharedUtil::AddDebugLog("WinHttpConnect failed: %lu", GetLastError());
        WinHttpCloseHandle(hSession);
        return false;
    }

    // Secure flag required for HTTPS
    HINTERNET hRequest =
        WinHttpOpenRequest(hConnect, L"POST", L"/anticheat/crash-report", NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
    if (!hRequest)
    {
        SharedUtil::AddDebugLog("WinHttpOpenRequest failed: %lu", GetLastError());
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return false;
    }

    const wchar_t* headers = L"Content-Type: application/json\r\n";
    BOOL           bSent = WinHttpSendRequest(hRequest, headers, -1, (LPVOID)strCrashReport.data(), dwDataSize, dwDataSize, 0);
    if (!bSent)
    {
        SharedUtil::AddDebugLog("WinHttpSendRequest failed: %lu", GetLastError());
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return false;
    }

    BOOL bReceived = WinHttpReceiveResponse(hRequest, NULL);
    if (!bReceived)
    {
        SharedUtil::AddDebugLog("WinHttpReceiveResponse failed: %lu", GetLastError());
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return false;
    }

    SharedUtil::AddDebugLog("HTTPS crash report sent successfully.");

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);


    return true;
}

jsoncons::json CCrashHandler::GenerateCrashReport(EXCEPTION_POINTERS* pException)
{
    jsoncons::json json;

    json["exception_code"] = "(Unknown)";
    json["exception_address"] = "(Unknown)";
    json["exception_flags"] = "(Unknown)";

    if (!pException || !pException->ExceptionRecord || !pException->ContextRecord)
    {
        json["error"] = "Invalid EXCEPTION_POINTERS";
        return json;
    }

    json["exception_code"] = pException->ExceptionRecord->ExceptionCode;
    json["exception_address"] = reinterpret_cast<uintptr_t>(pException->ExceptionRecord->ExceptionAddress);
    json["exception_flags"] = pException->ExceptionRecord->ExceptionFlags;

    CONTEXT* ctx = pException->ContextRecord;
    json["registers"] =
        jsoncons::json::object{{"RAX", static_cast<uint64_t>(ctx->Rax)}, {"RBX", static_cast<uint64_t>(ctx->Rbx)}, {"RCX", static_cast<uint64_t>(ctx->Rcx)},
                               {"RDX", static_cast<uint64_t>(ctx->Rdx)}, {"RSI", static_cast<uint64_t>(ctx->Rsi)}, {"RDI", static_cast<uint64_t>(ctx->Rdi)},
                               {"RSP", static_cast<uint64_t>(ctx->Rsp)}, {"RBP", static_cast<uint64_t>(ctx->Rbp)}, {"RIP", static_cast<uint64_t>(ctx->Rip)},
                               {"R8", static_cast<uint64_t>(ctx->R8)},   {"R9", static_cast<uint64_t>(ctx->R9)},   {"R10", static_cast<uint64_t>(ctx->R10)},
                               {"R11", static_cast<uint64_t>(ctx->R11)}, {"R12", static_cast<uint64_t>(ctx->R12)}, {"R13", static_cast<uint64_t>(ctx->R13)},
                               {"R14", static_cast<uint64_t>(ctx->R14)}, {"R15", static_cast<uint64_t>(ctx->R15)}};

    return json;
}