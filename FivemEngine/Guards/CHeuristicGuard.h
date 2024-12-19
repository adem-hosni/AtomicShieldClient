#pragma once
#include "StdInc.h"
#include "CGuardBase.h"

class CHeuristicGuard final : public CGuardBase
{
public:
    CHeuristicGuard();
    ~CHeuristicGuard();

    void AddSignatures(std::map<std::string, std::vector<std::string>>& Signatures);
    void DoPulse() override;

    unsigned int GetLatestScanResult() { return m_uiLatestScanResult; }
    void         UpdateLatestScanResult(unsigned int uiScanResult) { m_uiLatestScanResult = uiScanResult; }

private:
    unsigned int                                    m_uiLatestScanResult;
    std::map<std::string, std::vector<std::string>> m_Signatures;
};
