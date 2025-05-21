#pragma once
#include "StdInc.h"
#include "CGuardBase.h"

class CHeuristicGuard final : public CGuardBase
{
public:
    CHeuristicGuard();
    ~CHeuristicGuard();
    std::unordered_set<std::string> m_FivemModules = {
        "adhesive.dll", "botan.dll", "cfx_curl_x86_64.dll", "citizen-resources-client.dll", "citizen-resources-core.dll", "CoreRT.dll",
        "gta-core-five.dll", "http-client.dll", "legitimacy.dll", "mojo.dll", "net.dll", "net-base.dll", "net-http-server.dll", "net-tcp-server.dll",
        "pool-sizes-state.dll", "rage-allocator-five.dll", "rage-device-five.dll", "rage-graphics-five.dll", "rage-input-five.dll",
        "rage-nutsnbolts-five.dll", "rage-scripting-five.dll", "scripting-gta.dll", "steam.dll", "steam_api64.dll", "v8-9.3.345.16.dll", "vfs-core.dll",
        "d3d9.dll", "d3d10.dll", "d3d11.dll", "dxgi.dll", "XInput1_4.dll", "opengl32.dll", "glu32.dll",
        "nvgpucomp64.dll", "nvldumdx.dll", "nvppex.dll", "nvspcap64.dll", "nvwgf2umx.dll",
        "icui18n.dll", "icuuc.dll", "libuv.dll"};

    std::vector<DWORD64> m_vWhitelistedRegions;
    std::vector<DWORD64> m_vSuspiciousRegions;


    bool        IsWhitelistedModule(HANDLE hProcess, LPVOID address);
    void        Initialize() override;
    std::string GetScanProcessName() { return ""; }

    void        DoPulse();
    static void zebii();
    void        AddSignatures(std::map<std::string, std::vector<std::string>>& Signatures);
    static void StaticPulse(void* pContext) { reinterpret_cast<CHeuristicGuard*>(pContext)->DoPulse(); }
    void        ClearDetections() override {}

private:
    std::string              m_strScanProcessName;
    std::vector<std::string> m_vSignatures;
};