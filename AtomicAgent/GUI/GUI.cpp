#define IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DISABLE_DEBUG_TOOLS
#define IM_ASSERT(_EXPR) ((void)0)
#include "imgui.h"
#include "imgui_impl_dx9.h"
#include "imgui_internal.h"
#include "imgui_impl_win32.h"

#define IM_ASSERT(_EXPR) ((void)0)
#include "ImGuiSettings.h"

#include <d3d9.h>
#include <d3dx9.h>
#pragma comment(lib, "d3dx9.lib")
#include <tchar.h>
#include <thread>
#include <chrono>
#include <map>
#include <string>
#include <stdio.h>
#include <vector>
#include <time.h>
#include <ctime>
#include <dwmapi.h>
#include <codecvt>
#include <iomanip>
#include "StartUpManager.h"
#include "blur.hpp"
#include "GUI.h"
#include "Resources/Image.h"
#include "Resources/Font.h"
#include "Resources/resource.h"
#include "ImAnim/ImVec2Anim.h"
#include "ImAnim/ImVec4Anim.h"
#include "notification.h"
#include <CAtomicAPI.h>
#include <tlhelp32.h>
#include "EngineLauncher.h"
// Forward declarations of helper functions
bool           CreateDeviceD3D(HWND hWnd);
void           CleanupDeviceD3D();
void           ResetDevice();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

IDirect3DTexture9* bg = nullptr;

namespace image
{
    IDirect3DTexture9* Logo = nullptr;
    IDirect3DTexture9* Succes = nullptr;
    IDirect3DTexture9* exit = nullptr;
    IDirect3DTexture9* minimize = nullptr;
    IDirect3DTexture9* discord = nullptr;
    IDirect3DTexture9* youtube = nullptr;
    IDirect3DTexture9* tiktok = nullptr;
}            // namespace image

struct easingv2_state
{
    imanim::ImVec2Anim* anim = nullptr;
    ImVec2              current_vec;
};

// Debug options for CashLine text
struct CashLineDebug
{
    float  textSize = 72.345f;
    ImVec4 textColor = ImVec4(0.0f, 0.5f, 1.0f, 1.0f);
    ImVec2 positionOffset = ImVec2(-66.0f, 0.0f);
    ImVec2 boundingBoxSize = ImVec2(200.0f, 50.0f);
} cashlineDebug;

inline void EasingAnimationV2(std::string anim_name, ImVec2* current_vec, ImVec2 target_vec, float duration, imanim::EasingCurve::Type type, int loop)
{
    ImGuiWindow*  window = ImGui::GetCurrentWindow();
    const ImGuiID id = window->GetID(anim_name.c_str());

    static std::map<ImGuiID, easingv2_state> a;
    auto                                     it_a = a.find(id);

    if (it_a == a.end())
    {
        a.insert({id, easingv2_state()});
        it_a = a.find(id);
    }

    it_a->second.current_vec = *current_vec;

    if (it_a->second.anim == nullptr)
    {
        it_a->second.anim = new imanim::ImVec2Anim(current_vec);
        it_a->second.anim->setStartValue(it_a->second.current_vec);
        it_a->second.anim->setEndValue(target_vec);
        it_a->second.anim->setDuration(duration);
        it_a->second.anim->setLoopCount(loop);
        it_a->second.anim->setEasingCurve(type);
        it_a->second.anim->start();
    }
    else
    {
        it_a->second.anim->update();
        it_a->second.anim->setStartValue(it_a->second.current_vec);
        it_a->second.anim->setEndValue(target_vec);            // ?????????? ???????? ?????
    }
}

struct menu_anim
{
    float if_auth_offset;
    float offset_animation;
    float main_loading_offset;
    float auth_loading;
    float inject_alpha;
    float inject_progress;

    float size_window_x;
    float size_window_y;
} menu;

const char* cheat_name = "";

using namespace std;
using namespace std::chrono;
namespace texture
{
    IDirect3DTexture9* esp_preview = nullptr;
}
namespace esp_preview
{
    bool money = true;
    bool nickname = true;
    bool weapon = true;
    bool zoom = true;

    bool c4 = true;
    bool HP_line = true;
    bool hit = true;
    bool box = true;
    bool bomb = true;

    static float box_color[4] = {37 / 255.f, 37 / 255.f, 47 / 255.f, 1.f};
    static float nick_color[4] = {255 / 255.f, 255 / 255.f, 255 / 255.f, 1.f};
    static float money_color[4] = {255 / 255.f, 255 / 255.f, 255 / 255.f, 1.f};
    static float zoom_color[4] = {255 / 255.f, 255 / 255.f, 255 / 255.f, 1.f};
    static float c4_color[4] = {255 / 255.f, 255 / 255.f, 255 / 255.f, 1.f};
    static float bomb_color[4] = {255 / 255.f, 255 / 255.f, 255 / 255.f, 1.f};
    static float hp_color[4] = {255 / 255.f, 255 / 255.f, 255 / 255.f, 1.f};
    static float hp_line_color[4] = {112 / 255.f, 109 / 255.f, 214 / 255.f, 1.f};
    static float weapon_color[4] = {255 / 255.f, 255 / 255.f, 255 / 255.f, 1.f};
    static float hit_color[4] = {255 / 255.f, 255 / 255.f, 255 / 255.f, 1.f};

    int hp = 85;

}            // namespace esp_preview
HWND hwnd;
RECT rc;

float WIDTH = 662;             // Loader Size X
float HEIGHT = 500;            // Loader Size Y

ImVec2 menu_size = {WIDTH, HEIGHT};

void move_window()
{
    ImGui::SetCursorPos({0, 0});

    if (ImGui::InvisibleButton("Move_detector", ImVec2(menu_size)) || ImGui::IsItemActive())
    {
        GetWindowRect(hwnd, &rc);
        MoveWindow(hwnd, rc.left + ImGui::GetMouseDragDelta().x, rc.top + ImGui::GetMouseDragDelta().y, menu_size.x, menu_size.y, TRUE);
    }
}

bool Spinner(const char* label, float radius, int thickness, const ImU32& color)
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext&     g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID     id = window->GetID(label);

    ImVec2 pos = window->DC.CursorPos;
    ImVec2 size((radius) * 2, (radius) * 2);

    const ImRect bb(pos, ImVec2(pos.x + size.x, pos.y + size.y));
    ImGui::ItemSize(bb, style.FramePadding.y);
    if (!ImGui::ItemAdd(bb, id))
        return false;

    ImGui::GetWindowDrawList()->AddRect(bb.Min, bb.Max, ImGui::GetColorU32(c::bg_circle), 100.f, 0, thickness);

    ImGui::GetWindowDrawList()->PathClear();

    int num_segments = 360;
    int start = abs(ImSin(g.Time * 1.8f) * (num_segments - 5));

    const float a_min = IM_PI * 2.0f * ((float)start) / (float)num_segments;
    const float a_max = IM_PI * 2.0f * ((float)num_segments - 3) / (float)num_segments;

    const ImVec2 centre = ImVec2(pos.x + radius, pos.y + radius);

    for (int i = 0; i < num_segments; i++)
    {
        const float a = a_min + ((float)i / (float)num_segments) * (a_max - a_min);
        ImGui::GetWindowDrawList()->PathLineTo(ImVec2(centre.x + ImCos(a + g.Time * 8) * radius, centre.y + ImSin(a + g.Time * 8) * radius));
    }

    ImGui::GetWindowDrawList()->PathStroke(color, false, thickness);
}
float alpha = 0.6f;                    // ��������� �������� �����
float animationSpeed = 4.f;            // �������� �������� (��� ������, ��� ���������)

bool GUI::Initialize()
{
    GUI::wc.cbSize = sizeof(WNDCLASSEXW);
    GUI::wc.style = CS_CLASSDC;
    GUI::wc.lpfnWndProc = WndProc;
    GUI::wc.cbClsExtra = NULL;
    GUI::wc.cbWndExtra = NULL;
    GUI::wc.hCursor = LoadCursor(0, IDC_ARROW);
    GUI::wc.hbrBackground = nullptr;
    GUI::wc.lpszMenuName = L"Ceylon";
    GUI::wc.lpszClassName = L"Ceylon";
    GUI::wc.hInstance = GetModuleHandleW(NULL);
    GUI::wc.hIcon = LoadIcon(GUI::wc.hInstance, MAKEINTRESOURCE(ico4));
    GUI::wc.hIconSm = LoadIcon(GUI::wc.hInstance, MAKEINTRESOURCE(ico4));

    RegisterClassExW(&GUI::wc);
    hwnd = CreateWindowExW(NULL, GUI::wc.lpszClassName, L"Agent", WS_POPUP, (GetSystemMetrics(SM_CXSCREEN) / 2) - (WIDTH / 2),
                           (GetSystemMetrics(SM_CYSCREEN) / 2) - (HEIGHT / 2), WIDTH, HEIGHT, 0, 0, 0, 0);

    SetWindowLongA(hwnd, GWL_EXSTYLE, GetWindowLongW(hwnd, GWL_EXSTYLE) | WS_EX_LAYERED);
    SetLayeredWindowAttributes(hwnd, RGB(0, 0, 0), 255, LWA_ALPHA);

    MARGINS margins = {0, 0, 0, 0};
    DwmExtendFrameIntoClientArea(hwnd, &margins);

    POINT mouse;
    rc = {0};
    GetWindowRect(hwnd, &rc);

    if (!CreateDeviceD3D(hwnd))
    {
        CleanupDeviceD3D();
        ::UnregisterClassW(wc.lpszClassName, GUI::wc.hInstance);
        return false;
    }

    SetWindowRgn(hwnd, CreateRoundRectRgn(0, 0, WIDTH, HEIGHT, 30, 30), FALSE);

    ::ShowWindow(hwnd, SW_SHOWDEFAULT);
    ::ShowWindow(GetConsoleWindow(), SW_SHOWDEFAULT);
    ::UpdateWindow(hwnd);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void)io;

    ImFontConfig config2;

    ImGui::StyleColorsDark();

    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX9_Init(g_pd3dDevice);

    Quantico_Bold = io.Fonts->AddFontFromMemoryTTF(&Quantico_B, sizeof Quantico_B, 17.f, NULL, io.Fonts->GetGlyphRangesCyrillic());

    Quantico_Bold_1 = io.Fonts->AddFontFromMemoryTTF(&Quantico_B, sizeof Quantico_B, 44.f, NULL, io.Fonts->GetGlyphRangesCyrillic());

    Quantico_Bold_2 = io.Fonts->AddFontFromMemoryTTF(&Quantico_B, sizeof Quantico_B, 18.f, NULL, io.Fonts->GetGlyphRangesCyrillic());

    Quantico_Bold_3 = io.Fonts->AddFontFromMemoryTTF(&Quantico_B, sizeof Quantico_B, 15.f, NULL, io.Fonts->GetGlyphRangesCyrillic());

    Quantico_Regular = io.Fonts->AddFontFromMemoryTTF(&Quantico_R, sizeof Quantico_R, 14.f, NULL, io.Fonts->GetGlyphRangesCyrillic());

    Quantico_Regular_1 = io.Fonts->AddFontFromMemoryTTF(&Quantico_R, sizeof Quantico_R, 13.f, NULL, io.Fonts->GetGlyphRangesCyrillic());

    Instrument_Medium_2 = io.Fonts->AddFontFromMemoryTTF(&Instrument_M, sizeof Instrument_M, 15.f, NULL, io.Fonts->GetGlyphRangesCyrillic());

    Instrument_SemmiBold_1 = io.Fonts->AddFontFromMemoryTTF(&Instrument_S, sizeof Instrument_S, 19.f, NULL, io.Fonts->GetGlyphRangesCyrillic());

    Instrument_SemmiBold_2 = io.Fonts->AddFontFromMemoryTTF(&Instrument_S, sizeof Instrument_S, 35.f, NULL, io.Fonts->GetGlyphRangesCyrillic());

    Tektur_Medium = io.Fonts->AddFontFromMemoryTTF(&Tektur_M, sizeof Tektur_M, 36.f, NULL, io.Fonts->GetGlyphRangesCyrillic());

    Tektur_SemmiBold = io.Fonts->AddFontFromMemoryTTF(&Tektur_S, sizeof Tektur_S, 18.f, NULL, io.Fonts->GetGlyphRangesCyrillic());

    if (image::exit == nullptr)
        D3DXCreateTextureFromFileInMemoryEx(g_pd3dDevice, exit_icon, sizeof(exit_icon), 600, 600, D3DX_DEFAULT, 0, D3DFMT_UNKNOWN, D3DPOOL_MANAGED,
                                            D3DX_DEFAULT, D3DX_DEFAULT, 0, NULL, NULL, &image::exit);

    if (image::minimize == nullptr)
        D3DXCreateTextureFromFileInMemoryEx(g_pd3dDevice, minimize_icon, sizeof(minimize_icon), 600, 600, D3DX_DEFAULT, 0, D3DFMT_UNKNOWN, D3DPOOL_MANAGED,
                                            D3DX_DEFAULT, D3DX_DEFAULT, 0, NULL, NULL, &image::minimize);

    if (texture::esp_preview == nullptr)
        D3DXCreateTextureFromFileInMemoryEx(g_pd3dDevice, esp_preview1, sizeof(esp_preview1), 600, 600, D3DX_DEFAULT, 0, D3DFMT_UNKNOWN, D3DPOOL_MANAGED,
                                            D3DX_DEFAULT, D3DX_DEFAULT, 0, NULL, NULL, &texture::esp_preview);

    if (image::discord == nullptr)
        D3DXCreateTextureFromFileInMemoryEx(g_pd3dDevice, discrod_icon, sizeof(discrod_icon), 600, 600, D3DX_DEFAULT, 0, D3DFMT_UNKNOWN, D3DPOOL_MANAGED,
                                            D3DX_DEFAULT, D3DX_DEFAULT, 0, NULL, NULL, &image::discord);

    if (image::youtube == nullptr)
        D3DXCreateTextureFromFileInMemoryEx(g_pd3dDevice, youtube_icon, sizeof(youtube_icon), 600, 600, D3DX_DEFAULT, 0, D3DFMT_UNKNOWN, D3DPOOL_MANAGED,
                                            D3DX_DEFAULT, D3DX_DEFAULT, 0, NULL, NULL, &image::youtube);

    if (image::tiktok == nullptr)
        D3DXCreateTextureFromFileInMemoryEx(g_pd3dDevice, tiktok_icon, sizeof(tiktok_icon), 600, 600, D3DX_DEFAULT, 0, D3DFMT_UNKNOWN, D3DPOOL_MANAGED,
                                            D3DX_DEFAULT, D3DX_DEFAULT, 0, NULL, NULL, &image::tiktok);

    // CustomStyleColor();

    return true;
}

const char* fake_data = "This is a test string that will never be used but makes entropy lower.";
int         fake_function()
{
    return 42;
}

void GUI::RenderUI(bool* bInitialized, bool& bNoErrors, std::string* pstrErrorTitle, std::string* pstrErrorDescription, std::string processName, bool tos)
{
    bool               show_demo_window = true;
    bool               show_another_window = false;
    ImVec4             clear_color = c::color_bg_1;
    static bool        bDownloadStarted = false;
    static bool        bDownloadFinish = false;
    static float       fProgress = 0.f;
    static std::string strEngineBuffer;
    static bool        bInjected = false;
    static char        szLoadingMessage[144];
    static SUserData   DownloadData{};

    static bool s_bTosPopupOpen = tos;
    static bool bEnableStartup = StartupManager::IsAppInRegistry();

    static float anim_speed = ImGui::GetIO().DeltaTime * 12.f;
    ImGui::GetIO().IniFilename = NULL;
    ImGui::GetIO().LogFilename = NULL;
    bool done = false;
    while (!done)
    {
        MSG msg;
        while (::PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                done = true;
        }
        if (done)
            break;

        ImGui_ImplDX9_NewFrame();
        ImGui_ImplWin32_NewFrame();

        ImGui::NewFrame();
        {
            blur::set_device(g_pd3dDevice);
            blur::new_frame();
            ImGui::GetBackgroundDrawList()->AddImage(bg, ImVec2(0, 0), ImVec2(1920, 1080), ImVec2(0, 0), ImVec2(1, 1), ImColor(255, 255, 255, 255));
            ImGuiContext& g = *GImGui;
            ImGuiStyle*   style = &ImGui::GetStyle();
            style->Alpha = 1.0f;                                                          // No global transparency
            style->Colors[ImGuiCol_WindowBg] = ImVec4(0.1f, 0.1f, 0.1f, 1.0f);            // Fully opaque window background

            if (bg == nullptr)
                D3DXCreateTextureFromFileInMemoryEx(g_pd3dDevice, Background, sizeof(Background), 1920, 1080, D3DX_DEFAULT, 0, D3DFMT_UNKNOWN, D3DPOOL_MANAGED,
                                                    D3DX_DEFAULT, D3DX_DEFAULT, 0, NULL, NULL, &bg);

            CustomStyleColor();
            ImGui::SetNextWindowSize(ImVec2(WIDTH, HEIGHT));
            ImGui::SetNextWindowPos({0, 0});
            // Debug window

            ImGui::Begin("General", nullptr,
                         ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar);
            {
                auto draw = ImGui::GetWindowDrawList();

                const auto& p = ImGui::GetWindowPos();

                const ImVec2 &region = ImGui::GetContentRegionMax(), i = ImGui::GetStyle().ItemSpacing;

                if (image_bg == nullptr)
                    D3DXCreateTextureFromFileInMemoryEx(g_pd3dDevice, grid_image, sizeof(grid_image), 585, 500, D3DX_DEFAULT, 0, D3DFMT_UNKNOWN,
                                                        D3DPOOL_MANAGED, D3DX_DEFAULT, D3DX_DEFAULT, 0, NULL, NULL, &image_bg);
                ImGui::GetWindowDrawList()->AddImageRounded(image_bg, ImVec2(p.x, p.y), ImVec2(p.x + region.x, p.y + region.y), ImVec2(0, 0), ImVec2(1, 1),
                                                            ImGui::GetColorU32(c::image_bgs) /*color*/,            // Fully opaque color
                                                            20                                                     // rounding
                );

                blur::add_blur(ImGui::GetBackgroundDrawList(), p, ImVec2(p.x + region.x, p.y + region.y), 1.f);
                ImVec4 color(13.0f / 255.0f, 13.0f / 255.0f, 13.0f / 255.0f, 1.0f);

                // draw->AddRectFilled(ImVec2(p.x, p.y), ImVec2(p.x + region.x, p.y + region.y), ImGui::GetColorU32(color), 19);

                Trinage_background();

                tab_alpha = ImLerp(tab_alpha, (page == active_tab) ? 1.f : 0.f, 20.f * ImGui::GetIO().DeltaTime);
                if (tab_alpha < 0.01f && tab_add < 0.01f)
                    active_tab = page;

                static bool bTosAccepted = false;
                static bool bTosPopupOpened = false;

                if (tos && !bTosAccepted)
                {
                    if (!bTosPopupOpened)
                    {
                        ImGui::OpenPopup("Disclaimer and Legal Notice");
                        bTosPopupOpened = true;
                    }

                    const float popupWidth = 500.f;
                    const float popupHeight = 300.f;
                    ImGui::SetNextWindowSize(ImVec2(popupWidth, popupHeight));
                    ImGui::SetNextWindowPos(ImVec2((WIDTH - popupWidth) * 0.5f, (HEIGHT - popupHeight) * 0.5f));

                    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(25, 25));
                    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 40);
                    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
                    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 0.95f));
                    bool popupShown = ImGui::BeginPopupModal("Disclaimer and Legal Notice", nullptr,
                                                             ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove |
                                                                 ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar);

                    if (popupShown)
                    {
                        ImVec4 blue_color = ImVec4(0.0f, 0.5f, 1.0f, 1.0f);

                        ImGui::PushFont(Tektur_Medium);
                        ImGui::SetWindowFontScale(0.65f);            // 65% size

                        ImGui::SetCursorPosX((popupWidth - ImGui::CalcTextSize("Disclaimer and Legal Notice").x) * 0.5f);
                        ImGui::PushStyleColor(ImGuiCol_Text, blue_color);
                        ImGui::Text("Disclaimer and Legal Notice");
                        ImGui::SetWindowFontScale(1.0f);            // reset
                        ImGui::PopFont();
                        ImGui::PopStyleColor();

                        ImGui::Dummy(ImVec2(0.0f, 20.0f));

                        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
                        ImGui::PushFont(Instrument_Medium_2);

                        // First disclaimer line
                        ImGui::TextWrapped(
                            "This software is an independent tool intended for private use only. It is not affiliated with any"
                            "third-party services or platforms.");

                        // Manual layout — no TextWrapped
                        ImGui::Text("By clicking 'Accept' you agree to our");
                        ImGui::SameLine();

                        ImGui::PushStyleColor(ImGuiCol_Text, blue_color);
                        ImGui::Text("Terms of Use");
                        if (ImGui::IsItemHovered())
                            ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
                        if (ImGui::IsItemClicked())
                            OpenURL(skCrypt("https://atomic-shield.com/tos/"));
                        ImGui::PopStyleColor();
                        ImGui::SameLine();

                        ImGui::Text("and");
                        ImGui::SameLine();

                        ImGui::PushStyleColor(ImGuiCol_Text, blue_color);
                        ImGui::Text("General Conditions");
                        if (ImGui::IsItemHovered())
                            ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
                        if (ImGui::IsItemClicked())
                            OpenURL(skCrypt("https://atomic-shield.com/privacy/"));
                        ImGui::PopStyleColor();

                        ImGui::TextWrapped("Neither the developer nor the seller takes responsibility for any misuse.");

                        ImGui::PopFont();
                        ImGui::PopStyleColor();

                        ImGui::Dummy(ImVec2(0.0f, 15.0f));
                        ImGui::Dummy(ImVec2(0.0f, 10.0f));

                        ImGui::SetCursorPosX((popupWidth - 120) * 0.5f);
                        if (ImGui::ButtonLogins("Accept", ImVec2(120, 35)))
                        {
                            bTosAccepted = true;
                            SharedUtil::SetRegistryIntValue("AtomicShield_TOS", "AtomicShield_TOS", 1);

                            ImGui::CloseCurrentPopup();
                        }

                        ImGui::EndPopup();
                    }

                    // Always pop styles regardless of popup state
                    ImGui::PopStyleColor(1);
                    ImGui::PopStyleVar(3);
                    // Block all other UI interaction until accepted
                    ImGui::End();                 // Close the "General" window
                    ImGui::EndFrame();            // Skip rest of frame
                    g_pd3dDevice->SetRenderState(D3DRS_ZENABLE, FALSE);
                    g_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
                    g_pd3dDevice->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);

                    D3DCOLOR clear_col_dx = D3DCOLOR_RGBA((int)(clear_color.x * clear_color.w * 255.0f), (int)(clear_color.y * clear_color.w * 255.0f),
                                                          (int)(clear_color.z * clear_color.w * 255.0f), (int)(clear_color.w * 255.0f));

                    g_pd3dDevice->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, clear_col_dx, 1.0f, 0);
                    if (g_pd3dDevice->BeginScene() >= 0)
                    {
                        ImGui::RenderNotifications();
                        ImGui::Render();
                        ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
                        g_pd3dDevice->EndScene();
                    }

                    g_pd3dDevice->Present(NULL, NULL, NULL, NULL);
                    continue;            // Restart frame loop
                }
                static ImVec2 posmin(p.x + 105.54, p.y);
                static ImVec2 posmax(p.x + 573.18, p.y + 284.35);

                ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 1.0f);
                {
                    if (active_tab == 0)
                    {
                        if (image::Logo == nullptr)
                            D3DXCreateTextureFromFileInMemoryEx(g_pd3dDevice, Logo, sizeof(Logo), 500, 500, D3DX_DEFAULT, 0, D3DFMT_UNKNOWN, D3DPOOL_MANAGED,
                                                                D3DX_DEFAULT, D3DX_DEFAULT, 0, NULL, NULL, &image::Logo);
                        ImGui::GetWindowDrawList()->AddImageRounded(image::Logo, posmin, posmax, ImVec2(0, 0), ImVec2(1, 1),
                                                                    ImGui::GetColorU32(c::icon_welcome) /*color*/, 0 /*rounding*/);

                        ImGui::GetWindowDrawList()->AddText(Tektur_Medium,
                                                            cashlineDebug.textSize,            // <-- Replace with cashlineDebug.textSize
                                                            ImVec2(p.x + 250 + cashlineDebug.positionOffset.x, p.y + 101 + cashlineDebug.positionOffset.y),
                                                            ImGui::GetColorU32(c::text_blue), "CEYLON RP");

                        ImGui::GetWindowDrawList()->AddText(Tektur_Medium, 36.f, ImVec2(p.x + 380, p.y + 101), ImGui::GetColorU32(c::text_checkbox_active_on),
                                                            "");

                        if (bNoErrors)
                        {
                            ImGui::SetCursorPos(ImVec2(185, 285));
                            if (Custom_Checkbox("lbl", "Enable Startup Mode", &bEnableStartup, ImVec2(270, -1)))
                            {
                                if (bEnableStartup)
                                    StartupManager::AddAppToRegistry();
                                else
                                    StartupManager::RemoveAppFromRegistry();
                            }

                            if (ImGui::IsItemHovered())
                            {
                                ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10, 10));
                                ImGui::BeginTooltip();
                                ImGui::Text("Runs automatically at computer startup\nNo need to open the agent every time");
                                ImGui::EndTooltip();
                                ImGui::PopStyleVar();
                            }

                            ImGui::SetCursorPos(ImVec2(212, 314));
                            static const char* szLoadButtonText = SharedUtil::GetProcessID(skCrypt("AtomicSvc.exe")) ? "Restart" : "Start Now";

                            if (ImGui::ButtonLogins(szLoadButtonText, ImVec2(238, 40)))
                            {
                                if (!bDownloadFinish && !bDownloadStarted)
                                {
                                    std::thread EngineDownloaderThread(
                                        [&]()
                                        {
                                            bDownloadStarted = true;
                                            g_pAtomicAPI->DownloadEngine(&strEngineBuffer, &DownloadData);
                                            bDownloadFinish = true;
                                        });
                                    EngineDownloaderThread.detach();
                                }
                                page = 1, active_anim = true;
                            }
                        }
                        else
                        {
                            if (!*bInitialized)
                            {
                                ImGuiIO&    io = ImGui::GetIO();
                                ImDrawList* draw_list = ImGui::GetWindowDrawList();

                                ImVec2 window_pos = ImGui::GetWindowPos();
                                ImVec2 window_size = ImGui::GetWindowSize();

                                ImVec2 title_size = Tektur_Medium->CalcTextSizeA(36.f, FLT_MAX, 0.0f, pstrErrorTitle->c_str());
                                ImVec2 title_pos = window_pos + ImVec2((window_size.x - title_size.x) * 0.5f, 285.f);

                                ImVec2 desc_size = Instrument_Medium_2->CalcTextSizeA(15.f, FLT_MAX, 0.0f, pstrErrorDescription->c_str());
                                ImVec2 desc_pos = window_pos + ImVec2((window_size.x - desc_size.x) * 0.5f, 330.f);

                                draw_list->AddText(Tektur_Medium, 36.f, title_pos, ImGui::GetColorU32(c::text_blue), pstrErrorTitle->c_str());
                                draw_list->AddText(Instrument_Medium_2, 15.f, desc_pos, ImGui::GetColorU32(c::text), pstrErrorDescription->c_str());
                            }
                            else
                            {
                                ImGuiIO&    io = ImGui::GetIO();
                                ImDrawList* draw_list = ImGui::GetWindowDrawList();

                                const char* szLoadingMessage = skCrypt("Loading Content Manifest...");

                                ImVec2 window_pos = ImGui::GetWindowPos();
                                ImVec2 window_size = ImGui::GetWindowSize();

                                ImVec2 text_size = Tektur_Medium->CalcTextSizeA(36.f, FLT_MAX, 0.0f, szLoadingMessage);
                                ImVec2 center_pos = window_pos + ImVec2((window_size.x - text_size.x) * 0.5f, 310.f);

                                draw_list->AddText(Tektur_Medium, 36.f, center_pos, ImGui::GetColorU32(c::text_blue), szLoadingMessage);
                            }
                        }

                        ImGui::GetWindowDrawList()->AddRectFilled(ImVec2(p.x + 237, p.y + 415), ImVec2(p.x + 425, p.y + 416), ImGui::GetColorU32(c::line_bg),
                                                                  15.f);

                        ImGui::GetWindowDrawList()->AddText(Instrument_Medium_2, 15.f, ImVec2(p.x + 262, p.y + 377), ImGui::GetColorU32(c::text_button),
                                                            "Join our socials");
                        ImGui::GetWindowDrawList()->AddText(Instrument_Medium_2, 15.f, ImVec2(p.x + 355, p.y + 377), ImGui::GetColorU32(c::text_blue),
                                                            "network");

                        ImGui::SetCursorPos(ImVec2(287, 431));
                        ImGui::BeginGroup();
                        {
                            if (ImGui::Circle_icon("discord", image::discord, ImVec2(31, 31), ImGui::GetColorU32(c::multi_checkbox_blue), 0))
                            {
                                OpenURL(skCrypt("https://discord.ceylonroleplay.com/"));
                            }

                            ImGui::SameLine(0, 26);

                            if (ImGui::Circle_icon("youtube", image::youtube, ImVec2(31, 31), ImGui::GetColorU32(c::multi_checkbox_blue), 0))
                            {
                                OpenURL(skCrypt("https://www.youtube.com/@ceylonrpofficial"));
                            }
                        }
                        ImGui::EndGroup();
                    }
                    if (active_tab == 1)
                    {
                        ImVec4 color(13.0f / 255.0f, 13.0f / 255.0f, 13.0f / 255.0f, 1.0f);

                        draw->AddRectFilled(ImVec2(p.x, p.y), ImVec2(p.x + region.x, p.y + region.y), ImGui::GetColorU32(color), 10);

                        static ImVec2 product_offset_1(0.f, 550.f);

                        EasingAnimationV2("project_offset_1", &product_offset_1,
                                          active_anim == true    ? ImVec2(0.f, 0.f)
                                          : active_anim == false ? ImVec2(0, 500.f)
                                                                 : ImVec2(0, -500.f),
                                          1.f, imanim::EasingCurve::Type::InOutBack, -1);

                        draw->AddShadowCircle(ImVec2(p.x - 60, p.y + 260), 150.f, ImGui::GetColorU32(ImVec4(180 / 255.f, 218 / 255.f, 255 / 255.f, alpha)),
                                              300.f, ImVec2(0, 0));

                        draw->AddShadowCircle(ImVec2(p.x + region.x + 60, p.y + 260), 150.f,
                                              ImGui::GetColorU32(ImVec4(180 / 255.f, 218 / 255.f, 255 / 255.f, alpha)), 300.f, ImVec2(0, 0));

                        alpha = 0.5f + 0.5f * sinf(ImGui::GetTime() * animationSpeed);

                        ImGui::SetCursorPos(ImVec2(region) / 2 - ImVec2(80, 80 + product_offset_1.y));
                        Spinner("NULL", 80.f, 5.f, ImGui::GetColorU32(c::text_blue));

                        if (!strEngineBuffer.empty() && !bInjected)
                        {
                            memset(szLoadingMessage, 0, sizeof(szLoadingMessage));
                            strcpy(szLoadingMessage, "Loading AntiCheat...");

                            bInjected = true;
                            std::filesystem::path EnginePath = EngineLauncher::GetEnginePath();

                            if (SharedUtil::GetProcessID(skCrypt("AtomicSvc.exe")) != NULL)
                            {
                                SharedUtil::AddDebugLog("Engine detected, reloading...");
                                memset(szLoadingMessage, 0, sizeof(szLoadingMessage));
                                strcat(szLoadingMessage, skCrypt("CashLine is already running! Reloading"));
                                goto LOAD_ENGINE;
                            }
                            else
                            {
                                if (EngineLauncher::DumpEngineProcess(EnginePath, EngineLauncher::pProcessBuffer, sizeof(EngineLauncher::pProcessBuffer)))
                                {
                                    HANDLE                        hLauncher = INVALID_HANDLE_VALUE;
                                    EngineLauncher::eLaunchResult result = EngineLauncher::LaunchEngineProcess(EnginePath, &hLauncher);
                                    switch (result)
                                    {
                                        case EngineLauncher::eLaunchResult::UAC_CANCELLED:
                                            SharedUtil::AddDebugLog("UAC Cancelled");
                                            memset(szLoadingMessage, 0, sizeof(szLoadingMessage));
                                            strcat(szLoadingMessage, skCrypt("Operation aborted, Please accept to continue"));
                                            break;
                                        case EngineLauncher::eLaunchResult::LAUNCH_ELEVATION_FAILED:
                                            SharedUtil::AddDebugLog("Launch Elevation Failed");
                                            memset(szLoadingMessage, 0, sizeof(szLoadingMessage));
                                            strcat(szLoadingMessage, skCrypt("Failed to set anticheat rights!"));
                                            break;
                                        case EngineLauncher::eLaunchResult::SHELL_EXECUTE_FAILED:
                                            SharedUtil::AddDebugLog("Shell Execute Failed");
                                            memset(szLoadingMessage, 0, sizeof(szLoadingMessage));
                                            strcat(szLoadingMessage, skCrypt("Failed to execute anticheat launcher!"));
                                            break;
                                    }

                                    if (hLauncher == INVALID_HANDLE_VALUE || hLauncher == NULL)
                                    {
                                        SharedUtil::AddDebugLog("AntiCheat Launcher handle was null!");
                                        memset(szLoadingMessage, 0, sizeof(szLoadingMessage));
                                        strcat(szLoadingMessage, skCrypt("Failed to retreive anticheat launcher!"));
                                    }

                                    if (result == EngineLauncher::eLaunchResult::SUCCESS)
                                    {
                                    LOAD_ENGINE:
                                        int iInjectionResult = EngineLauncher::LoadEngineIntoLauncher(EnginePath, hLauncher, (BYTE*)strEngineBuffer.c_str(),
                                                                                                      strEngineBuffer.size());

                                        SharedUtil::AddDebugLog("Loading AntiCheat Result -> %d [Last Error: 0x%llx]", iInjectionResult, GetLastError());
                                        if (iInjectionResult == 0 || GetLastError() != ERROR_SUCCESS)
                                        {
                                            memset(szLoadingMessage, 0, sizeof(szLoadingMessage));
                                            strcat(szLoadingMessage, skCrypt("Please hold while we verify that everything is ready"));

                                            // Injection Succeded
                                            _beginthread(
                                                [](void*)
                                                {
                                                    time_t injected_time = time(NULL);
                                                    bool   bFailure = false;
                                                    while (!CheckIfLoaded("AtomicShield"))
                                                    {
                                                        // Wait 5 seconds if the 0 value didnt changed to 1, so the injection faileds
                                                        bFailure = time(NULL) - injected_time > 5;
                                                        if (bFailure)
                                                            break;

                                                        Sleep(90);
                                                    }

                                                    if (bFailure)
                                                    {
                                                        memset(szLoadingMessage, 0, sizeof(szLoadingMessage));
                                                        strcat(szLoadingMessage, skCrypt("Startup routine missing or blocked"));
                                                    }
                                                    else
                                                    {
                                                        SharedUtil::SetRegistryIntValue("AtomicShield", "AtomicShield", 0);
                                                        memset(szLoadingMessage, 0, sizeof(szLoadingMessage));
                                                        strcat(szLoadingMessage, skCrypt("Have Fun! [You can close now]"));
                                                        page = 2;
                                                        active_anim_1 = true;
                                                    }
                                                },
                                                0, nullptr);
                                        }
                                        else
                                        {
                                            memset(szLoadingMessage, 0, sizeof(szLoadingMessage));
                                            sprintf(szLoadingMessage, skCrypt("Failed to load anticheat (0x%X - 0x%X)"), iInjectionResult, GetLastError());
                                        }
                                    }
                                }
                                else
                                {
                                    memset(szLoadingMessage, 0, sizeof(szLoadingMessage));
                                    strcat(szLoadingMessage, skCrypt("Failed to load anticheat launcher!"));
                                }
                            }
                        }
                    }

                    if (active_tab == 2)
                    {
                        static ImVec2 product_offset_2(0.f, 550.f);

                        EasingAnimationV2("project_offset_2", &product_offset_2,
                                          active_anim_1 == true    ? ImVec2(0.f, 0.f)
                                          : active_anim_1 == false ? ImVec2(0, 500.f)
                                                                   : ImVec2(0, -500.f),
                                          1.f, imanim::EasingCurve::Type::InOutBack, -1);

                        if (image::Succes == nullptr)
                            D3DXCreateTextureFromFileInMemoryEx(g_pd3dDevice, Succes, sizeof(Succes), 500, 500, D3DX_DEFAULT, 0, D3DFMT_UNKNOWN,
                                                                D3DPOOL_MANAGED, D3DX_DEFAULT, D3DX_DEFAULT, 0, NULL, NULL, &image::Succes);
                        ImGui::GetWindowDrawList()->AddImageRounded(image::Succes, ImVec2(p.x + 246, p.y + 165 + product_offset_2.y),
                                                                    ImVec2(p.x + 415, p.y + 334 + product_offset_2.y), ImVec2(0, 0), ImVec2(1, 1),
                                                                    ImGui::GetColorU32(c::text_blue) /*color*/, 0 /*rounding*/);
                    }

                    // Check if the download is started and it's not finished yet
                    if (bDownloadStarted && !bDownloadFinish)
                    {
                        memset(szLoadingMessage, 0, sizeof(szLoadingMessage));
                        sprintf(szLoadingMessage, "Loading %d%%", (int)DownloadData.fProgress, "");
                    }

                    if (active_tab != 0)
                    {
                        ImGuiIO&    io = ImGui::GetIO();
                        ImDrawList* draw_list = ImGui::GetWindowDrawList();

                        ImVec2 text_size = Tektur_Medium->CalcTextSizeA(30.f, FLT_MAX, 0.0f, szLoadingMessage);
                        ImVec2 window_pos = ImGui::GetWindowPos();
                        ImVec2 window_size = ImGui::GetWindowSize();
                        ImVec2 center_pos = window_pos + ImVec2((window_size.x - text_size.x) * 0.5f, p.y + 410.0f);
                        draw_list->AddText(Tektur_Medium, 30.f, center_pos, ImGui::GetColorU32(c::text_blue), szLoadingMessage);
                    }

                    ImGui::SetCursorPos(ImVec2(region.x - 77, 12));
                    if (ImGui::Minimize_icon("minimize", image::minimize, ImVec2(27, 27), 0))
                    {
                        ShowWindow(hwnd, SW_MINIMIZE);
                    }

                    ImGui::SameLine(0, 7);
                    if (ImGui::Exit_icon("exit", image::exit, ImVec2(27, 27), 0))
                        exit(0);
                }
                ImGui::PopStyleVar();
                move_window();
            }
            ImGui::End();
        }

        ImGui::EndFrame();

        g_pd3dDevice->SetRenderState(D3DRS_ZENABLE, FALSE);
        g_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
        g_pd3dDevice->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
        D3DCOLOR clear_col_dx = D3DCOLOR_RGBA((int)(clear_color.x * clear_color.w * 255.0f), (int)(clear_color.y * clear_color.w * 255.0f),
                                              (int)(clear_color.z * clear_color.w * 255.0f), (int)(clear_color.w * 255.0f));
        g_pd3dDevice->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, clear_col_dx, 1.0f, 0);
        if (g_pd3dDevice->BeginScene() >= 0)
        {
            ImGui::RenderNotifications();
            ImGui::Render();
            ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());

            g_pd3dDevice->EndScene();
        }
        HRESULT result = g_pd3dDevice->Present(NULL, NULL, NULL, NULL);

        // Handle loss of D3D9 device
        if (result == D3DERR_DEVICELOST && g_pd3dDevice->TestCooperativeLevel() == D3DERR_DEVICENOTRESET)
            ResetDevice();
    }

    ImGui_ImplDX9_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
}

void GUI::Destroy()
{
    CleanupDeviceD3D();
    ::DestroyWindow(hwnd);
    ::UnregisterClassW(wc.lpszClassName, GUI::wc.hInstance);
}

bool CreateDeviceD3D(HWND hWnd)
{
    if ((g_pD3D = Direct3DCreate9(D3D_SDK_VERSION)) == NULL)
        return false;

    // Create the D3DDevice
    ZeroMemory(&g_d3dpp, sizeof(g_d3dpp));
    g_d3dpp.Windowed = TRUE;
    g_d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    g_d3dpp.BackBufferFormat = D3DFMT_UNKNOWN;            // Need to use an explicit format with alpha if needing per-pixel alpha composition.
    g_d3dpp.EnableAutoDepthStencil = TRUE;
    g_d3dpp.AutoDepthStencilFormat = D3DFMT_D16;
    g_d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_ONE;            // Present with vsync
    // g_d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;   // Present without vsync, maximum unthrottled framerate
    if (g_pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd, D3DCREATE_HARDWARE_VERTEXPROCESSING, &g_d3dpp, &g_pd3dDevice) < 0)
        return false;

    return true;
}

void CleanupDeviceD3D()
{
    if (g_pd3dDevice)
    {
        g_pd3dDevice->Release();
        g_pd3dDevice = NULL;
    }
    if (g_pD3D)
    {
        g_pD3D->Release();
        g_pD3D = NULL;
    }
}

void ResetDevice()
{
    ImGui_ImplDX9_InvalidateDeviceObjects();
    HRESULT hr = g_pd3dDevice->Reset(&g_d3dpp);
    if (hr != D3DERR_INVALIDCALL)
        ImGui_ImplDX9_CreateDeviceObjects();
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
        case WM_SIZE:
            if (g_pd3dDevice != NULL && wParam != SIZE_MINIMIZED)
            {
                g_d3dpp.BackBufferWidth = LOWORD(lParam);
                g_d3dpp.BackBufferHeight = HIWORD(lParam);
                ResetDevice();
            }
            return 0;
        case WM_SYSCOMMAND:
            if ((wParam & 0xfff0) == SC_KEYMENU)            // Disable ALT application menu
                return 0;
            break;
        case WM_DESTROY:
            ::PostQuitMessage(0);
            return 0;
    }
    return ::DefWindowProc(hWnd, msg, wParam, lParam);
}