#pragma once
#include "StdInc.h"
#include "CGuardBase.h"

class CHeuristicGuard final : public CGuardBase
{
public:
    CHeuristicGuard();
    ~CHeuristicGuard();
    
    void Initialize() override;

    void SpawnScanProcess();

    void        AddSignatures(std::map<std::string, std::unordered_set<std::string>>& Signatures);
    static void StaticPulse(void* pContext) { reinterpret_cast<CHeuristicGuard*>(pContext)->DoPulse(); }
    void        DoPulse() override {}

    unsigned int GetLatestScanResult() { return m_uiLatestScanResult; }
    void         UpdateLatestScanResult(unsigned int uiScanResult) { m_uiLatestScanResult = uiScanResult; }

private:
    unsigned int                                    m_uiLatestScanResult;
    //std::map<std::string, std::unordered_set<std::string>> m_Signatures;
};
