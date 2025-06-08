#pragma once
#include "StdInc.h"
#include "CGuardBase.h"

#define MANUALMAP_LOG(fmt, ...) SharedUtil::AddDebugLog("[ManualMappingGuard]: " fmt, __VA_ARGS__)

class CManualMappingGuard final : public CGuardBase
{
public:
    CManualMappingGuard();
    ~CManualMappingGuard();

    void Initialize() override;

    bool    IsModuleLoaded(DWORD64 dwAddress);
    DWORD64 GetPEHeaderSize(DWORD64 dwBaseAddress);

    void        DoPulse();
    static void StaticPulse(void* pContext) { reinterpret_cast<CManualMappingGuard*>(pContext)->DoPulse(); }
    void        ClearDetections() override {}
};
