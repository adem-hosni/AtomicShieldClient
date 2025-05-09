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
    static void zebii();
    void        AddSignatures(std::map<std::string, std::vector<std::string>>& Signatures);
    static void StaticPulse(void* pContext) { reinterpret_cast<CHeuristicGuard*>(pContext)->DoPulse(); }
    void        ClearDetections() override {  }

private:
    std::string              m_strScanProcessName;
    std::vector<std::string> m_vSignatures;
};