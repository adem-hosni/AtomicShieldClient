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
    CAhoCorasickMatcher*     m_pMatcher;
    std::string              m_strScanProcessName;
    std::vector<std::string> m_vSignatures;
    bool                     m_bFound;
};