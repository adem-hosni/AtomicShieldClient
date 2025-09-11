#pragma once
#include "imgui.h"

namespace c
{
    inline ImVec2 size = ImVec2(585, 500);

    // Base grays
    inline ImVec4 line_bg = ImColor(64, 64, 64, 255);                  // #404040
    inline ImVec4 text = ImColor(255, 255, 255, 255);                  // full white
    inline ImVec4 icon_welcome = ImColor(64, 64, 64, 22);              // #404040 alpha
    inline ImVec4 icon_user = ImColor(64, 64, 64, 148);                // #404040 alpha
    inline ImVec4 text_notif = ImColor(115, 115, 115, 255);            // #737373
    inline ImVec4 text_1 = ImColor(255, 255, 255, 127);
    inline ImVec4 image_dota = ImColor(64, 64, 64, 10);                 // #404040 alpha
    inline ImVec4 text_logo = ImColor(255, 255, 255, 255);              // brighter logo text
    inline ImVec4 text_in = ImColor(200, 200, 200, 255);                // brighter input text
    inline ImVec4 input_active = ImColor(64, 64, 64, 185);              // #404040 alpha
    inline ImVec4 input_inactive = ImColor(38, 38, 38, 255);            // #262626
    inline ImVec4 succes = ImColor(106, 255, 130, 255);
    inline ImVec4 image_bgs = ImColor(38, 38, 38, 9);            // #262626 alpha

    inline ImVec4 rect_multi_1 = ImColor(64, 64, 64, 165);            // #404040
    inline ImVec4 rect_multi = ImColor(38, 38, 38, 153);              // #262626
    inline ImVec4 rect_input = ImColor(64, 64, 64, 255);              // #404040
    inline ImVec4 rect_input_1 = ImColor(38, 38, 38, 166);            // #262626
    inline ImVec4 in_rect = ImColor(200, 200, 200, 186);              // brighter #737373 alpha

    inline ImVec4 text_page_login = ImColor(200, 200, 200, 193);            // brighter alpha

    inline ImVec4 accent = ImColor(200, 200, 200, 255);             // brighter accent
    inline ImVec4 outline = ImColor(64, 64, 64, 245);               // #404040
    inline ImVec4 background = ImColor(38, 38, 38, 245);            // #262626
    inline ImVec4 notif = ImColor(20, 20, 20, 120);
    inline ImVec4 color_bg = ImColor(26, 26, 26, 178);            // #262626 alpha

    inline ImVec4 color_bg_tab = ImColor(38, 38, 38, 89);                 // #262626 alpha
    inline ImVec4 color_bg_child = ImColor(38, 38, 38, 143);              // #262626 alpha
    inline ImVec4 color_rect_child = ImColor(64, 64, 64, 165);            // #404040
    inline ImVec4 color_text_child = ImColor(255, 255, 255, 140);
    inline ImVec4 color_rect_tab = ImColor(64, 64, 64, 63);            // #404040 alpha

    inline ImVec4 rect = ImColor(200, 200, 200, 76);            // brighter #737373 alpha
    inline ImVec4 rect_1 = ImColor(255, 255, 255, 15);
    inline ImVec4 color_bg_1 = ImColor(64, 64, 64, 51);            // #404040 alpha

    inline ImVec4 main_yellow = ImColor(200, 200, 200, 255);            // brighter for highlight

    inline ImVec4 line_tab = ImColor(64, 64, 64, 255);            // #404040
    inline ImVec4 black_rect = ImColor(0, 0, 0, 51);
    inline ImVec4 game_line_tab = ImColor(64, 64, 64, 255);            // #404040
    inline ImVec4 background_color = ImColor(0, 0, 0, 255);
    inline ImVec4 bg_spinner = ImColor(64, 64, 64, 140);            // #404040 alpha

    inline ImVec4 undetect = ImColor(115, 255, 141, 51);
    inline ImVec4 undetect_text = ImColor(115, 255, 141, 255);

    inline ImVec4 color_teg_text = ImColor(200, 200, 200, 255);            // brighter
    inline ImVec4 color_teg_bg = ImColor(64, 64, 64, 79);                  // #404040 alpha

    inline ImVec4 color_teg_text_1 = ImColor(255, 95, 95);
    inline ImVec4 color_teg_bg_1 = ImColor(255, 54, 58, 51);

    inline ImVec4 circle = ImColor(64, 64, 64, 255);              // #404040
    inline ImVec4 circle_in = ImColor(38, 38, 38, 79);            // #262626 alpha

    inline ImVec4 icon_tab_active = ImColor(200, 200, 200, 255);            // brighter #737373
    inline ImVec4 icon_tab_inactive = ImColor(64, 64, 64, 255);             // #404040

    inline ImVec4 background_tab_active = ImColor(64, 64, 64, 153);              // #404040 alpha
    inline ImVec4 background_tab_inactive = ImColor(26, 26, 26, 153);            // #262626 alpha

    inline ImVec4 rect_tab_active = ImColor(200, 200, 200, 216);            // brighter
    inline ImVec4 rect_tab_inactive = ImColor(38, 38, 38, 114);             // #262626 alpha

    inline ImVec4 shadow_tab_active = ImColor(200, 200, 200, 255);            // brighter
    inline ImVec4 shadow_tab_inactive = ImColor(0, 0, 0, 0);

    inline ImVec4 circle_icon = ImColor(200, 200, 200, 255);                     // brighter
    inline ImVec4 multi_checkbox_blue = ImColor(64, 64, 64, 255);                // #404040
    inline ImVec4 multi_checkbox_black = ImColor(38, 38, 38, 140);               // #262626 alpha
    inline ImVec4 multi_checkbox_in = ImColor(26, 26, 26, 255);                  // #262626
    inline ImVec4 multi_checkbox_hover = ImColor(200, 200, 200, 255);            // brighter

    inline ImVec4 circle_active = ImColor(0, 0, 0, 255);
    inline ImVec4 circle_inactive = ImColor(64, 64, 64, 255);            // #404040
    inline ImVec4 circle_hover = ImColor(200, 200, 200, 255);            // brighter
    inline ImVec4 rect_elements = ImColor(255, 255, 255, 3);
    inline ImVec4 rect_elements_2 = ImColor(38, 38, 38, 127);            // #262626 alpha

    inline ImVec4 text_checkbox_active = ImColor(255, 255, 255, 255);            // full white
    inline ImVec4 text_checkbox_hover = ImColor(245, 245, 245, 255);             // slightly off-white
    inline ImVec4 text_checkbox_active_on = ImColor(255, 255, 255, 255);
    inline ImVec4 text_checkbox_inactive = ImColor(200, 200, 200, 255);            // brighter
    inline ImVec4 text_checkbox_inactive_on = ImColor(211, 211, 211, 255);
    inline ImVec4 text_checkbox_inactive_hover = ImColor(180, 180, 180, 255);

    inline ImVec4 slider_multi_line = ImColor(200, 200, 200, 255);            // brighter
    inline ImVec4 slider_multi_line_1 = ImColor(0, 0, 0, 255);
    inline ImVec4 slider_rect = ImColor(255, 255, 255, 255);
    inline ImVec4 slider_rect_in = ImColor(26, 26, 26, 255);            // #262626

    inline ImVec4 combo_bg = ImColor(38, 38, 38, 165);                     // #262626 alpha
    inline ImVec4 combo_rect = ImColor(26, 26, 26, 127);                   // #262626 alpha
    inline ImVec4 combo_box = ImColor(64, 64, 64, 255);                    // #404040
    inline ImVec4 combo_bg_1 = ImColor(26, 26, 26, 255);                   // #262626
    inline ImVec4 combo_icon = ImColor(200, 200, 200, 255);                // brighter
    inline ImVec4 combo_icon_active = ImColor(64, 64, 64, 140);            // #404040 alpha

    inline ImVec4 selectable_bg = ImColor(38, 38, 38, 255);            // #262626

    inline ImVec4 color_picker_multi_1 = ImColor(64, 64, 64, 255);               // #404040
    inline ImVec4 color_picker_multi_2 = ImColor(200, 200, 200, 255);            // brighter

    inline ImVec4 rect_multi_green = ImColor(36, 255, 0, 255);
    inline ImVec4 rect_multi_red = ImColor(255, 21, 21, 255);
    inline ImVec4 rect_armor = ImColor(115, 151, 244, 255);

    inline ImVec4 text_button = ImColor(255, 255, 255, 255);            // full white
    inline ImVec4 text_blue = ImColor(255, 255, 255, 255);              // full white
    inline ImVec4 text_blue_1 = ImColor(200, 200, 200, 105);            // alpha
    inline ImVec4 bg_icon_button = ImColor(64, 64, 64, 255);            // #404040

    inline ImVec4 button_multi_1 = ImColor(200, 200, 200, 255);            // brighter
    inline ImVec4 button_multi_2 = ImColor(180, 180, 180, 255);            // lighter
    inline ImVec4 shadow_button = ImColor(38, 38, 38, 205);                // #262626 alpha

    inline ImVec4 bg_circle = ImColor(64, 64, 64, 14);                       // #404040 alpha
    inline ImVec4 bg_icon_profile = ImColor(38, 38, 38, 5);                  // #262626 alpha
    inline ImVec4 rect_icon_profile = ImColor(200, 200, 200, 20);            // brighter alpha
}            // namespace c

inline float   anim_speed = 12.f;
inline ImColor GetColorWithAlpha(ImColor color, float alpha)
{
    return ImColor(color.Value.x, color.Value.y, color.Value.z, alpha);
}
