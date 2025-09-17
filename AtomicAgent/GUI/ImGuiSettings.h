#pragma once
#include "imgui.h"

namespace c
{
    // Updated colors to blue theme (#00A2FF)
    static ImVec2 size = ImVec2(585, 500);
    static ImVec4 line_bg = ImColor(20, 30, 40, 255);            // Dark blue/gray
    static ImVec4 text = ImColor(255, 255, 255, 255);
    static ImVec4 icon_welcome = ImColor(0, 162, 255, 22);            // Blue with low alpha
    static ImVec4 icon_user = ImColor(0, 162, 255, 148);              // Blue
    static ImVec4 text_notif = ImColor(153, 153, 153, 255);
    static ImVec4 text_1 = ImColor(255, 255, 255, 127);
    static ImVec4 image_dota = ImColor(0, 162, 255, 10);            // Very transparent blue
    static ImVec4 text_logo = ImColor(227, 227, 227, 255);
    static ImVec4 text_in = ImColor(77, 75, 75, 255);
    static ImVec4 input_active = ImColor(0, 162, 255, 185);              // Blue
    static ImVec4 input_inactive = ImColor(0, 162, 255, 255);            // Blue
    static ImVec4 succes = ImColor(106, 255, 130, 255);
    static ImVec4 image_bgs = ImColor(0, 162, 255, 9);            // Very transparent blue

    static ImVec4 rect_multi_1 = ImColor(20, 30, 40, 165);            // Dark blue
    static ImVec4 rect_multi = ImColor(0, 0, 0, 153);
    static ImVec4 rect_input = ImColor(15, 22, 30, 255);              // Dark blue
    static ImVec4 rect_input_1 = ImColor(20, 30, 40, 166);            // Dark blue
    static ImVec4 in_rect = ImColor(25, 35, 45, 186);                 // Blue-ish dark

    static ImVec4 text_page_login = ImColor(80, 145, 170, 193);            // Light blue-ish

    static ImVec4 accent = ImColor(0, 162, 255);                    // Pure blue
    static ImVec4 outline = ImColor(20, 40, 60, 245);               // Dark blue/gray
    static ImVec4 background = ImColor(15, 25, 35, 245);            // Dark background
    static ImVec4 notif = ImColor(20, 20, 20, 120);
    static ImVec4 color_bg = ImColor(10, 18, 25, 178);            // Dark blue

    static ImVec4 color_bg_tab = ImColor(0, 0, 0, 89);
    static ImVec4 color_bg_child = ImColor(0, 0, 0, 143);
    static ImVec4 color_rect_child = ImColor(20, 30, 40, 165);            // Dark blue
    static ImVec4 color_text_child = ImColor(255, 255, 255, 140);
    static ImVec4 color_rect_tab = ImColor(20, 30, 40, 63);            // Dark blue

    static ImVec4 rect = ImColor(0, 162, 255, 76);            // Blue
    static ImVec4 rect_1 = ImColor(255, 255, 255, 15);
    static ImVec4 color_bg_1 = ImColor(40, 60, 70, 51);            // Blue-gray

    static ImVec4 main_blue = ImColor(0, 162, 255, 255);            // Pure blue

    static ImVec4 line_tab = ImColor(20, 40, 60, 255);            // Dark blue
    static ImVec4 black_rect = ImColor(0, 0, 0, 51);
    static ImVec4 game_line_tab = ImColor(20, 30, 40, 255);            // Dark blue
    static ImVec4 background_color = ImColor(0, 0, 0, 255);
    static ImVec4 bg_spinner = ImColor(25, 35, 45, 140);            // Blue-gray

    static ImVec4 undetect = ImColor(115, 255, 141, 51);
    static ImVec4 undetect_text = ImColor(115, 255, 141, 255);

    static ImVec4 color_teg_text = ImColor(0, 162, 255);              // Blue
    static ImVec4 color_teg_bg = ImColor(0, 162, 255, 79);            // Blue with alpha

    static ImVec4 color_teg_text_1 = ImColor(255, 95, 95);
    static ImVec4 color_teg_bg_1 = ImColor(255, 54, 58, 51);

    static ImVec4 circle = ImColor(0, 162, 255);                  // Blue
    static ImVec4 circle_in = ImColor(20, 30, 40, 79);            // Dark blue

    static ImVec4 icon_tab_active = ImColor(0, 162, 255, 255);            // Blue
    static ImVec4 icon_tab_inactive = ImColor(28, 28, 28, 255);

    static ImVec4 background_tab_active = ImColor(50, 95, 110, 153);            // Blue
    static ImVec4 background_tab_inactive = ImColor(12, 12, 12, 153);

    static ImVec4 rect_tab_active = ImColor(0, 162, 255, 216);            // Blue
    static ImVec4 rect_tab_inactive = ImColor(17, 17, 17, 114);

    static ImVec4 shadow_tab_active = ImColor(0, 162, 255, 255);            // Blue
    static ImVec4 shadow_tab_inactive = ImColor(0, 0, 0, 0);

    static ImVec4 circle_icon = ImColor(0, 255, 38, 255);
    static ImVec4 multi_checkbox_blue = ImColor(0, 162, 255, 255);            // Blue
    static ImVec4 multi_checkbox_black = ImColor(20, 30, 40, 140);            // Dark blue
    static ImVec4 multi_checkbox_in = ImColor(15, 25, 35, 255);               // Dark blue
    static ImVec4 multi_checkbox_hover = ImColor(25, 35, 45, 255);            // Blue-gray

    static ImVec4 circle_active = ImColor(0, 0, 0, 255);
    static ImVec4 circle_inactive = ImColor(25, 35, 45, 255);            // Blue-gray
    static ImVec4 circle_hover = ImColor(30, 45, 60, 255);               // Blue-gray
    static ImVec4 rect_elements = ImColor(255, 255, 255, 3);
    static ImVec4 rect_elements_2 = ImColor(15, 25, 35, 127);            // Dark blue

    static ImVec4 text_checkbox_active = ImColor(255, 255, 255, 142);
    static ImVec4 text_checkbox_hover = ImColor(255, 255, 255, 204);
    static ImVec4 text_checkbox_active_on = ImColor(255, 255, 255, 255);
    static ImVec4 text_checkbox_inactive = ImColor(110, 110, 130, 255);            // Blue-ish gray
    static ImVec4 text_checkbox_inactive_on = ImColor(211, 211, 211, 255);
    static ImVec4 text_checkbox_inactive_hover = ImColor(170, 170, 200, 255);            // Light blue-ish

    static ImVec4 slider_multi_line = ImColor(0, 162, 255, 255);            // Blue
    static ImVec4 slider_multi_line_1 = ImColor(0, 0, 0, 255);
    static ImVec4 slider_rect = ImColor(255, 255, 255, 255);
    static ImVec4 slider_rect_in = ImColor(10, 18, 25, 255);            // Dark blue

    static ImVec4 combo_bg = ImColor(20, 30, 40, 165);                      // Dark blue
    static ImVec4 combo_rect = ImColor(15, 25, 35, 127);                    // Dark blue
    static ImVec4 combo_box = ImColor(20, 30, 40, 255);                     // Dark blue
    static ImVec4 combo_bg_1 = ImColor(10, 18, 25, 255);                    // Very dark blue
    static ImVec4 combo_icon = ImColor(0, 120, 200, 255);                   // Blue
    static ImVec4 combo_icon_active = ImColor(0, 162, 255, 140);            // Blue

    static ImVec4 selectable_bg = ImColor(15, 22, 30, 255);            // Dark blue

    static ImVec4 color_picker_multi_1 = ImColor(20, 30, 40, 255);             // Dark blue
    static ImVec4 color_picker_multi_2 = ImColor(0, 162, 255, 255);            // Blue

    static ImVec4 rect_multi_green = ImColor(36, 255, 0, 255);
    static ImVec4 rect_multi_red = ImColor(255, 21, 21, 255);
    static ImVec4 rect_armor = ImColor(115, 151, 244, 255);

    static ImVec4 text_button = ImColor(80, 145, 170, 205);             // Blue-ish
    static ImVec4 text_blue = ImColor(0, 162, 255, 255);                // Blue
    static ImVec4 text_blue_1 = ImColor(0, 162, 255, 105);              // Blue with alpha
    static ImVec4 bg_icon_button = ImColor(20, 30, 40, 255);            // Dark blue

    static ImVec4 button_multi_1 = ImColor(0, 162, 255, 255);              // Blue
    static ImVec4 button_multi_2 = ImColor(100, 200, 255, 255);            // Light blue
    static ImVec4 shadow_button = ImColor(0, 162, 255, 205);               // Blue

    static ImVec4 bg_circle = ImColor(0, 162, 255, 14);                    // Very transparent blue
    static ImVec4 bg_icon_profile = ImColor(0, 162, 255, 5);               // Very transparent blue
    static ImVec4 rect_icon_profile = ImColor(0, 162, 255, 20);            // Blue with alpha
}            // namespace c

static float   anim_speed = 12.f;
static ImColor GetColorWithAlpha(ImColor color, float alpha)
{
    return ImColor(color.Value.x, color.Value.y, color.Value.z, alpha);
}