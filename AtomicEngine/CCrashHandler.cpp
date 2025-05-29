#include "StdInc.h"

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
    SharedUtil::AddDebugLog("Crash report generated successfully.");
    
    return g_pAtomicAPI->UploadCashReport(CrashReport);
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
    json["registers"] = jsoncons::json::object{{"RAX", static_cast<uint64_t>(ctx->Rax)}, {"RBX", static_cast<uint64_t>(ctx->Rbx)}, {"RCX", static_cast<uint64_t>(ctx->Rcx)},
                      {"RDX", static_cast<uint64_t>(ctx->Rdx)}, {"RSI", static_cast<uint64_t>(ctx->Rsi)}, {"RDI", static_cast<uint64_t>(ctx->Rdi)},
                      {"RSP", static_cast<uint64_t>(ctx->Rsp)}, {"RBP", static_cast<uint64_t>(ctx->Rbp)}, {"RIP", static_cast<uint64_t>(ctx->Rip)},
                      {"R8", static_cast<uint64_t>(ctx->R8)},   {"R9", static_cast<uint64_t>(ctx->R9)},   {"R10", static_cast<uint64_t>(ctx->R10)},
                      {"R11", static_cast<uint64_t>(ctx->R11)}, {"R12", static_cast<uint64_t>(ctx->R12)}, {"R13", static_cast<uint64_t>(ctx->R13)},
                      {"R14", static_cast<uint64_t>(ctx->R14)}, {"R15", static_cast<uint64_t>(ctx->R15)}};

    return json;
}