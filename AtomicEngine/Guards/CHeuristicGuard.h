#pragma once
#include "StdInc.h"
#include "CGuardBase.h"

class CHeuristicGuard final : public CGuardBase
{
public:
    CHeuristicGuard();
    ~CHeuristicGuard();
    std::unordered_set<std::wstring> m_whitelistedModules = {
        // FiveM core executables
        L"adhesive.dll", L"botan.dll", L"cfx_curl_x86_64.dll", L"citizen-resources-client.dll", L"citizen-resources-core.dll", L"CoreRT.dll",
        L"gta-core-five.dll", L"http-client.dll", L"legitimacy.dll", L"mojo.dll", L"net.dll", L"net-base.dll", L"net-http-server.dll", L"net-tcp-server.dll",
        L"pool-sizes-state.dll", L"rage-allocator-five.dll", L"rage-device-five.dll", L"rage-graphics-five.dll", L"rage-input-five.dll",
        L"rage-nutsnbolts-five.dll", L"rage-scripting-five.dll", L"scripting-gta.dll", L"steam.dll", L"steam_api64.dll", L"v8-9.3.345.16.dll", L"vfs-core.dll",

        // Common game dependencies
        L"d3d9.dll", L"d3d10.dll", L"d3d11.dll", L"dxgi.dll", L"XInput1_4.dll", L"opengl32.dll", L"glu32.dll",

        // NVIDIA graphics
        L"nvgpucomp64.dll", L"nvldumdx.dll", L"nvppex.dll", L"nvspcap64.dll", L"nvwgf2umx.dll",

        // System libraries commonly used by FiveM
        L"icui18n.dll", L"icuuc.dll", L"libuv.dll"};
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