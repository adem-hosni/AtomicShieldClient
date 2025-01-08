#pragma once
#include "StdInc.h"
#include "CGuardBase.h"

class CHeuristicGuard final : public CGuardBase
{
public:
    CHeuristicGuard();
    ~CHeuristicGuard();

    void Initialize() override;

    inline static void __fastcall ScanForString();

    void        AddSignatures(std::map<std::string, std::unordered_set<std::string>>& Signatures);
    static void StaticPulse(void* pContext) { reinterpret_cast<CHeuristicGuard*>(pContext)->DoPulse(); }
    void        DoPulse() override;

private:
    std::unordered_set<std::string> m_Signatures;
};
