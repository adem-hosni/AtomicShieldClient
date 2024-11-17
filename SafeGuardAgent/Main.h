#pragma once
#include "StdInc.h"

#define size_of IM_ARRAYSIZE

inline HWND hwnd;
inline RECT rc;

namespace images
{
    inline ID3D11ShaderResourceView* valo = nullptr;
    inline ID3D11ShaderResourceView* rust = nullptr;
    inline ID3D11ShaderResourceView* fn = nullptr;
    inline ID3D11ShaderResourceView* eft = nullptr;
    inline ID3D11ShaderResourceView* mw = nullptr;
    inline ID3D11ShaderResourceView* circle = nullptr;
}            // namespace images

namespace window
{
    const ImVec2 size_max = {625, 425};
    const float  rounding = 6.f;
}            // namespace window

namespace fonts
{
    inline ImFont* Inter_Regular = nullptr;
    inline ImFont* Sansation_Light = nullptr;
    inline ImFont* Sansation_Regular = nullptr;
    inline ImFont* Sansation_Bold = nullptr;
    inline ImFont* FontAwesome = nullptr;
}            // namespace fonts

namespace items
{
    const float rounding = 2.f;
}

namespace Colors
{
    const ImVec4 bg = {0.059f, 0.059f, 0.059f, .8f};
    const ImVec4 lbg = {0.086f, 0.086f, 0.086f, .95f};

    const ImVec4 SecondColor = {140 / 255.0f, 89 / 255.0f, 43 / 255.0f, .8f};
    const ImVec4 MainColor = {SecondColor.x, SecondColor.y, SecondColor.z, SecondColor.w * 3};
    const ImVec4 ItemBgColor = {0.1176f, 0.1176f, 0.1176f, 1.0f};

    const ImVec4 White = {1, 1, 1, 1};
    const ImVec4 lwhite = {1, 1, 1, 0.8};
    const ImVec4 Gray = {0.235f, 0.235f, 0.235f, 1};
    const ImVec4 DarkGray = {0.137, 0.137, 0.137, 1};
    const ImVec4 Green = {0.247, 1.0, 0.247, 1.0};
    const ImVec4 Red = {0.6824, 0.1608, 0.1608, 1.0};
    const ImVec4 Orange = {0.7451f, 0.6118f, 0.2471f, 1.0f};
}            // namespace Colors