#pragma once
#define IMGUI_DEFINE_MATH_OPERATORS
#include <d3d9.h>
#include "imgui_internal.h"
#include "ImGuiSettings.h"

static void CustomStyleColor()            // Отрисовка цветов
{
    ImGuiStyle&   s = ImGui::GetStyle();
    ImGuiContext& g = *GImGui;

    s.Colors[ImGuiCol_Border] = ImColor(0, 0, 0, 0);
    s.Colors[ImGuiCol_PopupBg] = ImColor(5, 5, 5, 255);
    s.Colors[ImGuiCol_PopupBg_1] = ImColor(7, 8, 18, 0);
    s.ChildRounding = 20.f;
    s.WindowRounding = 20.f;
    s.WindowPadding = ImVec2(0, 0);
    s.Colors[ImGuiCol_ChildBg] = ImColor(3, 3, 3, 255);
    s.Colors[ImGuiCol_WindowBg] = ImColor(2, 3, 9, 178);
}

inline ImFont* Quantico_Bold = nullptr;
inline ImFont* Quantico_Bold_1 = nullptr;
inline ImFont* Quantico_Bold_2 = nullptr;
inline ImFont* Quantico_Bold_3 = nullptr;
inline ImFont* Quantico_Regular = nullptr;
inline ImFont* Quantico_Regular_1 = nullptr;
inline ImFont* Instrument_Medium_2 = nullptr;
inline ImFont* Instrument_SemmiBold_1 = nullptr;
inline ImFont* Instrument_SemmiBold_2 = nullptr;
inline ImFont* Tektur_Medium = nullptr;
inline ImFont* Tektur_SemmiBold = nullptr;

static float tab_alpha = 0.f; /* */
static float tab_add;         /* */
static int   active_tab = 0;

static float tab_alphas = 0.f; /* */
static float tab_adds;         /* */
static int   active_tabs = 0;

static int tabs = 0, selector_tabs = 0;

static IDirect3DTexture9* star = nullptr;
static IDirect3DTexture9* image_bg = nullptr;
static IDirect3DTexture9* welcome_icon = nullptr;
static IDirect3DTexture9* user = nullptr;

static int     page = 0;
static bool    active_anim = false;
static bool    active_anim_1 = false;
static ImColor color_edit4_2 = {0 / 255.f, 132 / 255.f, 255 / 255.f, 1.f};

namespace ImGui
{
    static int  rotation_start_index;
    static void ImRotateStart()
    {
        rotation_start_index = ImGui::GetWindowDrawList()->VtxBuffer.Size;
    }

    static ImVec2 ImRotationCenter()
    {
        ImVec2 l(FLT_MAX, FLT_MAX), u(-FLT_MAX, -FLT_MAX);            // bounds

        const auto& buf = ImGui::GetWindowDrawList()->VtxBuffer;
        for (int i = rotation_start_index; i < buf.Size; i++)
            l = ImMin(l, buf[i].pos), u = ImMax(u, buf[i].pos);

        return ImVec2((l.x + u.x) / 2, (l.y + u.y) / 2);            // or use _ClipRectStack?
    }

    static void ImRotateEnd(float rad, ImVec2 center = ImRotationCenter())
    {
        float s = sin(rad), c = cos(rad);
        center = ImRotate(center, s, c) - center;

        auto& buf = ImGui::GetWindowDrawList()->VtxBuffer;
        for (int i = rotation_start_index; i < buf.Size; i++)
            buf[i].pos = ImRotate(buf[i].pos, s, c) - center;
    }
}            // namespace ImGui

static void Trinage_background()
{
    static ImVec2 screen_size = {(float)GetSystemMetrics(SM_CXSCREEN), (float)GetSystemMetrics(SM_CYSCREEN)};

    static ImVec2 partile_pos[100];
    static ImVec2 partile_target_pos[100];
    static float  partile_speed[100];
    static float  partile_size[100];
    static float  partile_radius[100];
    static float  partile_rotate[100];

    for (int i = 1; i < 50; i++)
    {
        if (partile_pos[i].x == 0 || partile_pos[i].y == 0)
        {
            partile_pos[i].x = rand() % (int)screen_size.x + 1;
            partile_pos[i].y = 15.f;
            partile_speed[i] = 1 + rand() % 25;
            partile_radius[i] = rand() % 4;
            partile_size[i] = rand() % 8;

            partile_target_pos[i].x = rand() % (int)screen_size.x;
            partile_target_pos[i].y = screen_size.y * 2;
        }

        partile_pos[i] = ImLerp(partile_pos[i], partile_target_pos[i], ImGui::GetIO().DeltaTime * (partile_speed[i] / 60));
        partile_rotate[i] += ImGui::GetIO().DeltaTime;

        if (partile_pos[i].y > screen_size.y)
        {
            partile_pos[i].x = 0;
            partile_pos[i].y = 0;
            partile_rotate[i] = 0;
        }

        ImGui::ImRotateStart();
        ImGui::GetWindowDrawList()->AddCircleFilled(partile_pos[i], partile_size[i], ImGui::GetColorU32(c::text_blue), 1);
        ImGui::ImRotateEnd(partile_rotate[i]);
    }
}

namespace GUI
{
    static WNDCLASSEXW wc;

    bool Initialize();
    void RenderUI(bool bNoErrors, std::string strErrorTitle, std::string strErrorDescription, std::string processName);
    void Destroy();
}            // namespace GUI