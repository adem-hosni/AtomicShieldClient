#pragma once
#include "StdInc.h"
#include "CGuardBase.h"

class CHeuristicGuard final : public CGuardBase
{
public:
    CHeuristicGuard();
    ~CHeuristicGuard();

    void Initialize() override;

    void        AddSignatures(std::map<std::string, std::vector<std::wstring>>& Signatures);
    static void StaticPulse(void* pContext) { reinterpret_cast<CHeuristicGuard*>(pContext)->DoPulse(); }
    static void __fastcall DoPulse();

private:
    std::unordered_set<std::string> m_Signatures;
};
