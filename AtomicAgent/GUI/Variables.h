#pragma once

#include "imgui.h"
#include <string>
#include <vector>
#include <map>

struct IDirect3DDevice9;
struct IDirect3DTexture9;
struct ID3D11ShaderResourceView;
struct ID3D11Device;
struct ID3D11DeviceContext;
struct IDXGISwapChain;

class c_variables
{
public:
    struct
    {
        ImGuiWindowFlags flags = ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoBackground;
        ImVec2 size = ImVec2(740, 530);
        ImVec2 padding = ImVec2(0, 0);
        ImVec2 spacing = ImVec2(10, 10);
        float shadow_size = 0.f;
        float border_size = 0.f;
        float rounding = 4.f;
        ImVec2 logo_size = ImVec2(80, 80);
        ImVec2 section_size = ImVec2(80, 340);
        ImVec2 child_select_size = ImVec2(270, 80);
        ImVec2 gui_dpi_size = ImVec2(255, 40);
        ImVec2 hue_slider_size = ImVec2(370, 30);
        ImVec2 topbar_size = ImVec2(size.x - section_size.x - spacing.x, 30);
        float scrollbar_size = 4.f;
    } window;

    struct
    {
        ImFont* inter[4];
        ImFont* icons[10];
    } font;

    struct
    {
        float content_alpha = 0.f;

        int section_count = 0;
        const char* section_icons[5] = { "C", "A", "D", "B", "E" };

        bool favorite_childs = false;
        std::map<ImGuiID, bool> child_states;

        std::string cheat_name = ".gg/atomic-shield";
        std::string cheat_year = "Make closet cheating great again";
    } gui;

    float dpi = 1.f;
    int stored_dpi = 100;
    bool dpi_changed = true;

    IDirect3DDevice9* device_dx9 = nullptr;
    ID3D11Device* device_dx11 = nullptr;
    ID3D11DeviceContext* device_context = nullptr;
    IDXGISwapChain* swap_chain = nullptr;
};

inline c_variables* var = new c_variables();
