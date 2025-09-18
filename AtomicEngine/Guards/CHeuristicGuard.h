#pragma once
#include "StdInc.h"
#include "CGuardBase.h"

class CHeuristicGuard final : public CGuardBase
{
public:
    struct RegionHashEntry
    {
        PVOID        baseAddress{};
        SIZE_T       regionSize{};
        DWORD        protect{};
        XXH64_hash_t hash{};
    };

    CHeuristicGuard();
    ~CHeuristicGuard();

    void        Initialize() override;
    std::string GetScanProcessName() { return ""; }

    void        DoPulse();
    void        AddSignatures(std::map<std::string, std::vector<std::string>>& Signatures);
    static void StaticPulse(void* pContext) { reinterpret_cast<CHeuristicGuard*>(pContext)->DoPulse(); }
    void        ClearDetections() override;

    bool IsHeartbeatActive() { return m_tLastHeartbeat == NULL || time(NULL) - m_tLastHeartbeat < 35; }

private:
    std::map<LPVOID, DWORD64>    m_WhitelistedRegions;
    std::map<LPVOID, DWORD64>    m_BlacklistedRegions;
    std::string                  m_strScanProcessName;
    std::vector<std::string>     m_vSignatures;
    std::atomic<bool>            m_bFound;
    std::vector<RegionHashEntry> m_vScannedRegions;
    time_t                       m_tLastHeartbeat;
};