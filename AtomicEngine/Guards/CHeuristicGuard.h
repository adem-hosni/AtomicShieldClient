#pragma once
#include "StdInc.h"
#include "CGuardBase.h"

class CHeuristicGuard final : public CGuardBase
{
public:
    CHeuristicGuard();
    ~CHeuristicGuard();

    void Initialize() override;

    std::string BuildSignatureParameters();
    void        SpawnScanProcess();
    std::string GetScanProcessName();

    void DoPulse() {}

    void        AddSignatures(std::map<std::string, std::vector<std::wstring>>& Signatures);
    static void StaticPulse(void* pContext) { reinterpret_cast<CHeuristicGuard*>(pContext)->DoPulse(); }

private:
    std::string              m_strScanProcessName;
    std::vector<std::string> m_Signatures;
};