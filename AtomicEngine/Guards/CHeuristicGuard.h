#pragma once
#include "StdInc.h"
#include "CGuardBase.h"

class CHeuristicGuard final : public CGuardBase
{
public:
    CHeuristicGuard();
    ~CHeuristicGuard();

    void        Initialize() override;
    std::string GetScanProcessName() { return ""; }

    void        DoPulse();
    void        AddSignatures(std::map<std::string, std::vector<std::string>>& Signatures);
    static void StaticPulse(void* pContext) { reinterpret_cast<CHeuristicGuard*>(pContext)->DoPulse(); }
    void        ClearDetections() override { m_bFound = false; }

private:
    std::map<LPVOID, DWORD64> m_WhitelistedRegions;
    std::map<LPVOID, DWORD64> m_BlacklistedRegions;
    std::string               m_strScanProcessName;
    std::vector<std::string>  m_vSignatures;
    std::atomic<bool>         m_bFound;
};