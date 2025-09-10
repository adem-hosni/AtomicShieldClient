#pragma once
#include "imgui.h"

namespace c
{
    // Updated colors to gold theme
    inline ImVec2 size = ImVec2(585, 500);
    inline ImVec4 line_bg = ImColor(40, 35, 25, 255);            // Dark gold/brown
    inline ImVec4 text = ImColor(255, 255, 255, 255);
    inline ImVec4 icon_welcome = ImColor(255, 215, 0, 22);            // Gold with low alpha
    inline ImVec4 icon_user = ImColor(255, 215, 0, 148);              // Gold
    inline ImVec4 text_notif = ImColor(153, 153, 153, 255);
    inline ImVec4 text_1 = ImColor(255, 255, 255, 127);
    inline ImVec4 image_dota = ImColor(255, 215, 0, 10);            // Very transparent gold
    inline ImVec4 text_logo = ImColor(227, 227, 227, 255);
    inline ImVec4 text_in = ImColor(77, 75, 75, 255);
    inline ImVec4 input_active = ImColor(255, 215, 0, 185);              // Gold
    inline ImVec4 input_inactive = ImColor(255, 215, 0, 255);            // Gold
    inline ImVec4 succes = ImColor(106, 255, 130, 255);
    inline ImVec4 image_bgs = ImColor(255, 215, 0, 9);            // Very transparent gold

    inline ImVec4 rect_multi_1 = ImColor(40, 35, 25, 165);            // Dark gold/brown
    inline ImVec4 rect_multi = ImColor(0, 0, 0, 153);
    inline ImVec4 rect_input = ImColor(25, 22, 15, 255);              // Dark gold/brown
    inline ImVec4 rect_input_1 = ImColor(40, 35, 25, 166);            // Dark gold/brown
    inline ImVec4 in_rect = ImColor(60, 50, 30, 186);                 // Gold-ish brown

    inline ImVec4 text_page_login = ImColor(170, 145, 80, 193);            // Gold-ish

    inline ImVec4 accent = ImColor(255, 215, 0);                    // Pure gold
    inline ImVec4 outline = ImColor(65, 55, 35, 245);               // Dark gold/brown
    inline ImVec4 background = ImColor(30, 25, 15, 245);            // Dark brown
    inline ImVec4 notif = ImColor(20, 20, 20, 120);
    inline ImVec4 color_bg = ImColor(15, 13, 8, 178);            // Dark brown

    inline ImVec4 color_bg_tab = ImColor(0, 0, 0, 89);
    inline ImVec4 color_bg_child = ImColor(0, 0, 0, 143);
    inline ImVec4 color_rect_child = ImColor(40, 35, 25, 165);            // Dark gold/brown
    inline ImVec4 color_text_child = ImColor(255, 255, 255, 140);
    inline ImVec4 color_rect_tab = ImColor(40, 35, 25, 63);            // Dark gold/brown

    inline ImVec4 rect = ImColor(170, 145, 80, 76);            // Gold
    inline ImVec4 rect_1 = ImColor(255, 255, 255, 15);
    inline ImVec4 color_bg_1 = ImColor(70, 60, 40, 51);            // Gold-brown

    inline ImVec4 main_yellow = ImColor(255, 215, 0, 255);            // Pure gold

    inline ImVec4 line_tab = ImColor(65, 55, 35, 255);            // Dark gold/brown
    inline ImVec4 black_rect = ImColor(0, 0, 0, 51);
    inline ImVec4 game_line_tab = ImColor(40, 35, 25, 255);            // Dark gold/brown
    inline ImVec4 background_color = ImColor(0, 0, 0, 255);
    inline ImVec4 bg_spinner = ImColor(60, 50, 30, 140);            // Gold-brown

    inline ImVec4 undetect = ImColor(115, 255, 141, 51);
    inline ImVec4 undetect_text = ImColor(115, 255, 141, 255);

    inline ImVec4 color_teg_text = ImColor(255, 215, 0);              // Gold
    inline ImVec4 color_teg_bg = ImColor(255, 215, 0, 79);            // Gold with alpha

    inline ImVec4 color_teg_text_1 = ImColor(255, 95, 95);
    inline ImVec4 color_teg_bg_1 = ImColor(255, 54, 58, 51);

    inline ImVec4 circle = ImColor(255, 215, 0);                  // Gold
    inline ImVec4 circle_in = ImColor(40, 35, 25, 79);            // Dark gold/brown

    inline ImVec4 icon_tab_active = ImColor(255, 215, 0, 255);            // Gold
    inline ImVec4 icon_tab_inactive = ImColor(28, 28, 28, 255);

    inline ImVec4 background_tab_active = ImColor(110, 95, 50, 153);            // Gold
    inline ImVec4 background_tab_inactive = ImColor(12, 12, 12, 153);

    inline ImVec4 rect_tab_active = ImColor(255, 215, 0, 216);            // Gold
    inline ImVec4 rect_tab_inactive = ImColor(17, 17, 17, 114);

    inline ImVec4 shadow_tab_active = ImColor(255, 215, 0, 255);            // Gold
    inline ImVec4 shadow_tab_inactive = ImColor(0, 0, 0, 0);

    inline ImVec4 circle_icon = ImColor(0, 255, 38, 255);
    inline ImVec4 multi_checkbox_blue = ImColor(255, 215, 0, 255);            // Gold
    inline ImVec4 multi_checkbox_black = ImColor(40, 35, 25, 140);            // Dark gold/brown
    inline ImVec4 multi_checkbox_in = ImColor(30, 25, 15, 255);               // Dark brown
    inline ImVec4 multi_checkbox_hover = ImColor(50, 45, 30, 255);            // Gold-brown

    inline ImVec4 circle_active = ImColor(0, 0, 0, 255);
    inline ImVec4 circle_inactive = ImColor(60, 50, 30, 255);            // Gold-brown
    inline ImVec4 circle_hover = ImColor(70, 60, 40, 255);               // Gold-brown
    inline ImVec4 rect_elements = ImColor(255, 255, 255, 3);
    inline ImVec4 rect_elements_2 = ImColor(30, 25, 15, 127);            // Dark brown

    inline ImVec4 text_checkbox_active = ImColor(255, 255, 255, 142);
    inline ImVec4 text_checkbox_hover = ImColor(255, 255, 255, 204);
    inline ImVec4 text_checkbox_active_on = ImColor(255, 255, 255, 255);
    inline ImVec4 text_checkbox_inactive = ImColor(110, 100, 80, 255);            // Gold-ish gray
    inline ImVec4 text_checkbox_inactive_on = ImColor(211, 211, 211, 255);
    inline ImVec4 text_checkbox_inactive_hover = ImColor(170, 160, 140, 255);            // Light gold-ish

    inline ImVec4 slider_multi_line = ImColor(255, 215, 0, 255);            // Gold
    inline ImVec4 slider_multi_line_1 = ImColor(0, 0, 0, 255);
    inline ImVec4 slider_rect = ImColor(255, 255, 255, 255);
    inline ImVec4 slider_rect_in = ImColor(20, 18, 12, 255);            // Dark brown

    inline ImVec4 combo_bg = ImColor(35, 30, 20, 165);                      // Dark brown
    inline ImVec4 combo_rect = ImColor(30, 25, 15, 127);                    // Dark brown
    inline ImVec4 combo_box = ImColor(40, 35, 25, 255);                     // Dark gold/brown
    inline ImVec4 combo_bg_1 = ImColor(15, 13, 8, 255);                     // Very dark brown
    inline ImVec4 combo_icon = ImColor(150, 130, 80, 255);                  // Gold
    inline ImVec4 combo_icon_active = ImColor(255, 215, 0, 140);            // Gold

    inline ImVec4 selectable_bg = ImColor(25, 22, 15, 255);            // Dark brown

    inline ImVec4 color_picker_multi_1 = ImColor(40, 35, 25, 255);             // Dark gold/brown
    inline ImVec4 color_picker_multi_2 = ImColor(110, 95, 50, 255);            // Gold

    inline ImVec4 rect_multi_green = ImColor(36, 255, 0, 255);
    inline ImVec4 rect_multi_red = ImColor(255, 21, 21, 255);
    inline ImVec4 rect_armor = ImColor(115, 151, 244, 255);

    inline ImVec4 text_button = ImColor(170, 145, 80, 205);             // Gold
    inline ImVec4 text_blue = ImColor(255, 228, 80, 255);                // Gold (replacing blue)
    inline ImVec4 text_blue_1 = ImColor(255, 215, 0, 105);              // Gold with alpha
    inline ImVec4 bg_icon_button = ImColor(40, 35, 25, 255);            // Dark gold/brown

    inline ImVec4 button_multi_1 = ImColor(255, 215, 0, 255);              // Gold
    inline ImVec4 button_multi_2 = ImColor(255, 230, 150, 255);            // Light gold
    inline ImVec4 shadow_button = ImColor(255, 215, 0, 205);               // Gold

    inline ImVec4 bg_circle = ImColor(255, 215, 0, 14);                    // Very transparent gold
    inline ImVec4 bg_icon_profile = ImColor(255, 215, 0, 5);               // Very transparent gold
    inline ImVec4 rect_icon_profile = ImColor(255, 215, 0, 20);            // Gold with alpha
}            // namespace c

inline float anim_speed = 12.f;
inline ImColor GetColorWithAlpha(ImColor color, float alpha)
{
    return ImColor(color.Value.x, color.Value.y, color.Value.z, alpha);
}
