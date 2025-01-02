#pragma once

#define IMGUI_DEFINE_MATH_OPERATORS

#include "imgui.h"
#include "imgui_internal.h"
#include "Variables.h"
#include "Elements.h"
#include <vector>
#include <sstream>
#include <string>

using namespace ImGui;

#define SCALE(...) scale_impl(__VA_ARGS__, var->dpi)

inline ImVec2 scale_impl(const ImVec2& vec, float dpi) {
    return ImVec2(roundf(vec.x * dpi), roundf(vec.y * dpi));
}

inline ImVec2 scale_impl(float x, float y, float dpi) {
    return ImVec2(roundf(x * dpi), roundf(y * dpi));
}

inline float scale_impl(float var, float dpi) {
    return roundf(var * dpi);
}

struct keybind_state
{
    int mode = 0;
    int key = 0;
    bool value = false;
};

class c_gui
{
public:


    template <typename T>
    T* anim_container(T** state_ptr, ImGuiID id)
    {
        T* state = static_cast<T*>(GetStateStorage()->GetVoidPtr(id));
        if (!state)
            GetStateStorage()->SetVoidPtr(id, state = new T());
        else if (state)
            GetStateStorage()->GetVoidPtr(id);

        *state_ptr = state;
        return state;
    }

    float fixed_speed(float speed) { return speed / ImGui::GetIO().Framerate; };

   

    bool color_edit(std::string_view label, float col[4], ImGuiColorEditFlags flags = 0);

  

};

inline c_gui* gui = new c_gui();

enum fade_direction : int
{
    vertically,
    horizontally,
    diagonally,
    diagonally_reversed,
};

class c_draw
{
public:
    ImU32 get_clr(const ImVec4& col, float alpha = 1.f);

    void rect_filled_multi_color(ImDrawList* draw, const ImVec2& p_min, const ImVec2& p_max, ImU32 col_upr_left, ImU32 col_upr_right, ImU32 col_bot_right, ImU32 col_bot_left, float rounding = 0.f, ImDrawFlags flags = 0);

    void fade_rect_filled(ImDrawList* draw, const ImVec2& pos_min, const ImVec2& pos_max, ImU32 col_one, ImU32 col_two, fade_direction direction, float rounding = 0.f, ImDrawFlags flags = 0);

    void render_text(ImDrawList* draw_list, ImFont* font, const ImVec2& pos_min, const ImVec2& pos_max, ImU32 color, const char* text, const char* text_display_end = NULL, const ImVec2* text_size_if_known = NULL, const ImVec2& align = ImVec2(0.f, 0.f), const ImRect* clip_rect = NULL);

    void radial_gradient(ImDrawList* draw_list, const ImVec2& center, float radius, ImU32 col_in, ImU32 col_out);

    void set_linear_color_alpha(ImDrawList* draw_list, int vert_start_idx, int vert_end_idx, ImVec2 gradient_p0, ImVec2 gradient_p1, ImU32 col0, ImU32 col1);

   // void watermark(std::string_view name, std::string_view fps, std::string_view ping, std::string_view time);
};

inline c_draw* draw = new c_draw();

//struct notify_state
//{
//    int notify_id;
//    std::string_view text;
//    bool warning;
//    float notify_alpha = 0.f;
//    float notify_offset = 0.f;
//    bool active_notify = true;
//    float notify_timer = 0.f;
//};
//
//class c_notify
//{
//public:
//    void setup_notify();
//
//    void add_notify(std::string_view text, bool warning);
//
//private:
//    void render_notify(int cur_notify_value, float notify_alpha, float notify_offset, float notify_percentage, std::string_view text, bool warnin);
//
//    float notify_time = 15.f;
//
//    int notify_count = 0;
//
//    std::vector<notify_state> notifications;
//
//};
//
//inline c_notify* notify = new c_notify();
