#include "CGUI.h"
#include "Resources/images.h"
#include "Resources/fonts.h"
#include "Resources/font_awesome.h"
#include "ManualMapInjector.hpp"
#include "particles.h"
#include <map>
#include <vector>

DWORD window_flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoBackground;

//UI ITEMS
CGUI::CUIElements c;
CGUI::GUI g;

void CGUI::GUI::begin(const char* name, ImVec2 size)
{
    ImGuiStyle& s = ImGui::GetStyle();

    s.WindowRounding = window::rounding;
    s.Alpha = 1;

    ImGui::Begin(name, NULL, window_flags);
    ImGui::SetWindowSize(size);
    ImGuiWindow* window = ImGui::GetCurrentWindow();

    window->DrawList->AddRectFilled({ 0,0 }, ImVec2(size.x - 1, size.y - 1), ImGui::GetColorU32(Colors::bg), window::rounding);
    ImGui::PopStyleVar(1);
    style();
    mw();
}

void CGUI::GUI::end()
{
    ImGui::End();
}

void CGUI::GUI::ChildBegin(const char* name, float posX, float posY, float sizeX, float sizeY)
{
    auto s = ImGui::GetStyle();
    ImGui::SetNextWindowPos({ posX, posY });

    ImGui::BeginChild(name, { sizeX, sizeY });
}
void CGUI::GUI::ChildEnd() 
{
    ImGui::EndChild();
}

void CGUI::GUI::style()
{
    ImGuiStyle& style = ImGui::GetStyle();

    style.WindowBorderSize = 0.f;
    style.WindowRounding = window::rounding;
    style.ChildRounding = window::rounding;
    style.FrameBorderSize = 0.f;
    style.FrameRounding = items::rounding;
    style.WindowPadding = ImVec2(0, 0);
    style.Colors[ImGuiCol_ChildBg] = Colors::lbg;
    style.ScrollbarSize = 5.f;
    style.ScrollbarRounding = 10;
    style.ItemSpacing = { 10, 15 };
}

void CGUI::GUI::Blur(HWND hwnd)
{
    struct ACCENTPOLICY
    {
        int na;
        int nf;
        int nc;
        int nA;
    };
    struct WINCOMPATTRDATA
    {
        int na;
        PVOID pd;
        ULONG ul;
    };

    const HINSTANCE hm = LoadLibrary("user32.dll");
    if (hm)
    {
        typedef BOOL(WINAPI* pSetWindowCompositionAttribute)(HWND, WINCOMPATTRDATA*);

        const pSetWindowCompositionAttribute SetWindowCompositionAttribute = (pSetWindowCompositionAttribute)GetProcAddress(hm, "SetWindowCompositionAttribute");
        if (SetWindowCompositionAttribute)
        {
            ACCENTPOLICY policy = { 4, 0, 155, 0 }; // 4,0,155,0 (Acrylic blur) 3,0,0,0 
            WINCOMPATTRDATA data = { 19, &policy,sizeof(ACCENTPOLICY) };
            SetWindowCompositionAttribute(hwnd, &data);
        }
        FreeLibrary(hm);
    }
}

void CGUI::GUI::mw()
{
    GetWindowRect(hwnd, &rc);
    MoveWindow(hwnd, rc.left + ImGui::GetWindowPos().x, rc.top + ImGui::GetWindowPos().y, window::size_max.x, window::size_max.y, TRUE);
    ImGui::SetWindowPos(ImVec2(0.f, 0.f));
}

void CGUI::GUI::pf(ImFont* font)
{
    ImGui::PushFont(font);
}

void CGUI::GUI::AddImage(ID3D11ShaderResourceView* pic, float posX, float posY, float sizeX, float sizeY)
{
    ImGui::SetCursorPos({ posX, posY });
    ImGui::Image(pic, { sizeX, sizeY });
}

void CGUI::GUI::AddImageRotated(ID3D11ShaderResourceView* pic, float posX, float posY, float sizeX, float sizeY)
{
    auto style = ImGui::GetStyle();
    style.Alpha = alphaColor;

    ImGui::SetCursorPos({ posX, posY });
    ImGui::ImageRotation(pic, {sizeX, sizeY}, ImVec2(1, 1), ImVec2(0, 0), ImGui::GetColorU32(Colors::MainColor), 0.1f);
}

void CircleImage(ID3D11ShaderResourceView* user_texture_id, const ImVec2& size, const ImVec2& uv0 = ImVec2(0, 0), const ImVec2& uv1 = ImVec2(1, 1), const ImVec4& tint_col = ImVec4(1, 1, 1, 1))
{
    auto style = ImGui::GetStyle();
    style.Alpha = alphaColor;

    ImVec2 current_cursor_pos = ImGui::GetCursorPos();
    ImGui::SetCursorPos(current_cursor_pos);

    ImVec2 p_min = ImGui::GetCursorScreenPos();
    ImVec2 p_max = ImVec2(p_min.x + size.x, p_min.y + size.y);
    float rounding = 5.0f;

    ImGui::GetWindowDrawList()->AddImageRounded(user_texture_id, p_min, p_max, uv0, uv1, ImGui::GetColorU32(tint_col), rounding);
    ImGui::Dummy(size);

    ImGui::SetCursorPos(current_cursor_pos);
}

void CGUI::GUI::ApplyImage(int i)
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    auto style = ImGui::GetStyle();
    style.Alpha = alphaColor;

    ImVec2 MIN = ImGui::GetItemRectMin();
    ImVec2 MAX = ImGui::GetItemRectMax();

    window->DrawList->AddRectFilledMultiColor({ MIN.x, MIN.y }, { MAX.x - 160, MAX.y }, ImGui::GetColorU32(Colors::bg), ImColor(0, 0, 0, 0), ImColor(0, 0, 0, 0), ImGui::GetColorU32(Colors::bg));
}

//Framework::CUSTOM ITEMS

struct ButtonState {
    ImVec4 ButtonColor;
    ImVec4 BorderColor;
    ImVec4 ShadowColor;
    ImVec4 TextColor;
    float timer;
    ButtonState() : ButtonColor(0, 0, 0, 0), BorderColor(0, 0, 0, 0), TextColor(0, 0, 0, 0), timer(0.0f) {}
};
bool CGUI::CUIElements::Button(const char* label, const char* text, float posX, float posY, float sizeX, float sizeY) {
    ImGuiStyle& style = ImGui::GetStyle();
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    ImGuiIO& io = ImGui::GetIO();
    style.Alpha = alphaColor;

    ImGui::SetCursorPos({ posX, posY });
    bool result = ImGui::InvisibleButton(label, { sizeX, sizeY });

    ImGuiID id = window->GetID(label);
    static std::map <ImGuiID, ButtonState> anim;
    auto it_anim = anim.find(id);

    if (it_anim == anim.end()) {
        anim.insert({ id, ButtonState() });
        it_anim = anim.find(id);
    }

    float time = io.DeltaTime;

    ImVec4 target = ImGui::IsItemActive() ? Colors::MainColor : ImGui::IsItemHovered() ? Colors::MainColor: Colors::SecondColor;
    ImVec4 ShadowColorTarget = ImGui::IsItemActive() ? Colors::MainColor : ImGui::IsItemHovered() ? Colors::SecondColor : ImVec4(0, 0, 0, 0);
    ImVec4 targetText = ImGui::IsItemActive() || ImGui::IsItemHovered() ? Colors::White : Colors::lwhite;

    it_anim->second.ButtonColor = ImVec4(ImLerp(it_anim->second.ButtonColor.x, target.x, time * 6), ImLerp(it_anim->second.ButtonColor.y, target.y, time * 6), ImLerp(it_anim->second.ButtonColor.z, target.z, time * 6), ImLerp(it_anim->second.ButtonColor.w, target.w, time * 6));
    it_anim->second.TextColor = ImVec4(ImLerp(it_anim->second.TextColor.x, targetText.x, time * 6), ImLerp(it_anim->second.TextColor.y, targetText.y, time * 6), ImLerp(it_anim->second.TextColor.z, targetText.z, time * 6), ImLerp(it_anim->second.TextColor.w, targetText.w, time * 6));
    it_anim->second.ShadowColor = ImVec4(ImLerp(it_anim->second.ShadowColor.x, ShadowColorTarget.x, time * 6), ImLerp(it_anim->second.ShadowColor.y, ShadowColorTarget.y, time * 6), ImLerp(it_anim->second.ShadowColor.z, ShadowColorTarget.z, time * 6), ImLerp(it_anim->second.ShadowColor.w, ShadowColorTarget.w / 2, time * 6));

    ImVec2 MIN = ImGui::GetItemRectMin();
    ImVec2 MAX = ImGui::GetItemRectMax();

    window->DrawList->AddRectFilled(MIN, MAX, ImGui::GetColorU32(it_anim->second.ButtonColor), items::rounding, 0);
    window->DrawList->AddShadowRect(MIN, MAX, ImGui::GetColorU32(it_anim->second.ShadowColor), 50, ImVec2(0,0), 0, 3.f);

    if (text) {
        ImVec2 textSize = ImGui::CalcTextSize(text);
        ImVec2 textPos = { MIN.x + (sizeX - textSize.x) * 0.5f, MIN.y + (sizeY - textSize.y) * 0.5f };
        window->DrawList->AddText(textPos, ImGui::GetColorU32(it_anim->second.TextColor), text);
    }

    return result;
}

struct TabState {
    ImVec4 TabColor;
    ImVec4 TextColor;
    ImVec4 IconColor;
    ImVec4 ShadowColor;
};

bool CGUI::CUIElements::Tab(const char* label, const char* text, const char* icon, float posX, float posY, float sizeX, float sizeY, bool v)
{
    ImGuiStyle& style = ImGui::GetStyle();
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    ImGuiIO& io = ImGui::GetIO();
    style.Alpha = alphaColor;

    ImGui::SetCursorPos({ posX, posY });
    bool result = ImGui::InvisibleButton(label, { sizeX, sizeY });

    ImGuiID id = window->GetID(label);
    static std::map<ImGuiID, TabState> anim;
    auto it_anim = anim.find(id);

    if (it_anim == anim.end()) {
        anim.insert({ id, TabState() });
        it_anim = anim.find(id);
    }

    float time = io.DeltaTime;

    if (result)
        v = !v;

    ImVec4 targetText = v? Colors::White : ImGui::IsItemHovered() ? Colors::lwhite : Colors::Gray;
   
    it_anim->second.TextColor = ImVec4(ImLerp(it_anim->second.TextColor.x, targetText.x, time * 6), ImLerp(it_anim->second.TextColor.y, targetText.y, time * 6), ImLerp(it_anim->second.TextColor.z, targetText.z, time * 6), ImLerp(it_anim->second.TextColor.w, targetText.w, time * 6));
    
    ImVec2 MIN = ImGui::GetItemRectMin();
    ImVec2 MAX = ImGui::GetItemRectMax();


    if (text) {
        ImVec2 textSize = ImGui::CalcTextSize(text);
        ImVec2 textPos = { MIN.x, MIN.y + (sizeY - textSize.y) * 0.5f };
        window->DrawList->AddText(textPos, ImGui::GetColorU32(it_anim->second.TextColor), text);
    }

    return result;
}

struct SelectableItemState {
    ImVec4 ShadowColor;
    ImVec4 FilledColor;
};

bool CGUI::CUIElements::SelectableItem(const char* label, ID3D11ShaderResourceView* pic, float posX, float posY, float sizeX, float sizeY, bool v)
{
    CGUI::GUI gui;
    ImGuiStyle& style = ImGui::GetStyle();
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    ImGuiIO& io = ImGui::GetIO();
    style.Alpha = alphaColor;

    ImGui::SetCursorPos({ posX, posY });
    bool result = ImGui::InvisibleButton(label, { sizeX, sizeY });

    ImGuiID id = window->GetID(label);
    static std::map<ImGuiID, SelectableItemState> anim;
    auto it_anim = anim.find(id);

    if (it_anim == anim.end()) {
        anim.insert({ id, SelectableItemState() });
        it_anim = anim.find(id);
        it_anim->second.FilledColor = ImVec4(1, 1, 1, 1);
    }

    float time = io.DeltaTime;

    if (result)
        v = !v;

    ImVec4 ShadowColorTarget = v ? Colors::MainColor : ImGui::IsItemHovered() ? Colors::SecondColor : ImVec4(0, 0, 0, 0);
    ImVec4 FilledColorTarget = v ? ImVec4(0, 0, 0, 0) : ImGui::IsItemHovered() ? ImVec4(0.1, 0.1, 0.1, 0.5) : ImVec4(0.1, 0.1, 0.1, 0.7);

    it_anim->second.ShadowColor = ImVec4(ImLerp(it_anim->second.ShadowColor, ShadowColorTarget, time * 6));
    it_anim->second.FilledColor = ImVec4(ImLerp(it_anim->second.FilledColor, FilledColorTarget, time * 6));

    ImVec2 MIN = ImGui::GetItemRectMin();
    ImVec2 MAX = ImGui::GetItemRectMax();

    ImVec2 p_min = { MIN.x, MIN.y };
    ImVec2 p_max = { MAX.x, MAX.y };

    window->DrawList->AddShadowRect(p_min, p_max, ImGui::GetColorU32(it_anim->second.ShadowColor), 15, ImVec2(0, 0), 0, 3.f);

    window->DrawList->AddImage((void*)pic, p_min, p_max);
    window->DrawList->AddRectFilled({p_min.x - 1, p_min.y - 1}, p_max, ImGui::GetColorU32(it_anim->second.FilledColor), items::rounding);

    return result;
}

struct Checkbox2State {
    ImVec4 ButtonColor;
    ImVec4 FilledRectColor;
    ImVec4 TextColor;
    
    ImVec2 Button_Pos;
};

bool CGUI::CUIElements::Checkbox2(const char* label, const char* text, float posX, float posY, bool* v, float space)
{
    ImGuiStyle& style = ImGui::GetStyle();
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    ImGuiIO& io = ImGui::GetIO();
    style.Alpha = alphaColor;

    ImGui::SetCursorPos({ posX, posY });
    bool result = ImGui::InvisibleButton(label, { 25, 10 });

    ImVec2 MIN = ImGui::GetItemRectMin();
    ImVec2 MAX = ImGui::GetItemRectMax();

    ImGuiID id = window->GetID(label);
    static std::map<ImGuiID, Checkbox2State> anim;
    auto it_anim = anim.find(id);

    if (it_anim == anim.end()) {
        anim.insert({ id, Checkbox2State() });
        it_anim = anim.find(id);
        it_anim->second.Button_Pos.x = MIN.x;
        it_anim->second.TextColor = *v ? Colors::White : Colors::Gray;
        it_anim->second.FilledRectColor = *v ? Colors::DarkGray : ImVec4(.071f, 0.071f, 0.071f, 1.0f);
        it_anim->second.ButtonColor = *v ? Colors::MainColor : Colors::Gray;
    }

    float time = io.DeltaTime * 6;

    if (result)
        *v = !*v;

    ImVec4 Button_Target = *v ? Colors::MainColor : Colors::Gray;
    ImVec4 FilledRect_Target = *v ? Colors::DarkGray : ImVec4(.071f, 0.071f, 0.071f, 1.0f);
    ImVec4 Text_Target = *v ? Colors::White : ImGui::IsItemHovered() ? Colors::lwhite : Colors::Gray;
    ImVec2 ButtonPos_Target = *v ? ImVec2(MAX.x - 12, (MAX.y + MIN.y) / 2 - 6) : ImVec2(MIN.x, (MAX.y + MIN.y) / 2 - 6);

    it_anim->second.ButtonColor = ImVec4(ImLerp(it_anim->second.ButtonColor, Button_Target, time));
    it_anim->second.FilledRectColor = ImVec4(ImLerp(it_anim->second.FilledRectColor, FilledRect_Target, time));
    it_anim->second.TextColor = ImVec4(ImLerp(it_anim->second.TextColor, Text_Target, time));
    it_anim->second.Button_Pos = ImLerp(it_anim->second.Button_Pos, ButtonPos_Target, time);

    window->DrawList->AddRectFilled(MIN, MAX, ImGui::GetColorU32(it_anim->second.FilledRectColor), 2.f);

    if (text) {
        ImVec2 textSize = ImGui::CalcTextSize(text);
        ImVec2 textPos = { MIN.x + (MAX.x - MIN.x - textSize.x) * 0.5f, MIN.y + (MAX.y - MIN.y - textSize.y) * 0.5f };
        window->DrawList->AddText({ textPos.x - space, textPos.y }, ImGui::GetColorU32(it_anim->second.TextColor), text);
    }

    ImVec2 buttonMin = { it_anim->second.Button_Pos.x, (MAX.y + MIN.y) / 2 - 6 };
    ImVec2 buttonMax = { buttonMin.x + 12, buttonMin.y + 12 };
    window->DrawList->AddRectFilled(buttonMin, buttonMax, ImGui::GetColorU32(it_anim->second.ButtonColor), 2.f);

    return result;
}

struct InputState {
    ImVec4 TextColor;
    ImVec4 ShadowColor;
    float Shadow_size;
};

bool CGUI::CUIElements::InputText(const char* label, const char* hint, float posX, float posY, float sizeX, static char buf[], size_t buf_size, ImGuiInputTextFlags flag)
{
    ImGuiStyle& style = ImGui::GetStyle();
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    ImGuiIO& io = ImGui::GetIO();

    style.Colors[ImGuiCol_FrameBg] = ImVec4(0, 0, 0, 0);
    style.FramePadding = ImVec2(0, 5);
    style.Alpha = alphaColor;


    ImGui::PushFont(fonts::Inter_Regular);
    ImGui::SetNextItemWidth(sizeX);
    ImGui::SetCursorPos({ posX, posY });

    ImGuiID id = window->GetID(label);
    static std::map<ImGuiID, InputState> anim;
    auto it_anim = anim.find(id);
    float time = io.DeltaTime;

    if (it_anim == anim.end()) {
        anim.insert({ id, InputState() });
        it_anim = anim.find(id);
        it_anim->second.TextColor = Colors::Gray;
        it_anim->second.ShadowColor = Colors::SecondColor;
    }

    style.Colors[ImGuiCol_TextDisabled] = it_anim->second.TextColor;

    ImVec2 MIN = ImVec2(posX, posY);
    ImVec2 MAX = ImVec2(posX + sizeX, posY + io.Fonts->Fonts[0]->FontSize + style.FramePadding.y * 2);

    window->DrawList->AddShadowRect({MIN.x, MAX.y - 1}, MAX, ImGui::GetColorU32(it_anim->second.ShadowColor), it_anim->second.Shadow_size, {0,0}, 0, items::rounding);
    window->DrawList->AddLine({ MIN.x, MAX.y }, { MAX.x, MAX.y }, ImGui::GetColorU32(Colors::MainColor), 1.f);

    bool result = ImGui::InputTextEx(label, hint, buf, (int)buf_size, ImVec2(0, 0), flag, 0, 0); //LINE 5063

    ImVec4 TextTarget = ImGui::IsItemActive() ? Colors::White : ImGui::IsItemHovered() ? Colors::White : Colors::Gray;
    ImVec4 ShadowTarget = ImGui::IsItemActive() ? Colors::MainColor : Colors::SecondColor;

    float Target_Size = ImGui::IsItemActive() ? 25.f : ImGui::IsItemHovered() ? 20.f : 5.f;

    it_anim->second.Shadow_size = ImLerp(it_anim->second.Shadow_size, Target_Size, time);
    it_anim->second.TextColor = ImVec4(ImLerp(it_anim->second.TextColor.x, TextTarget.x, time), ImLerp(it_anim->second.TextColor.y, TextTarget.y, time), ImLerp(it_anim->second.TextColor.z, TextTarget.z, time), ImLerp(it_anim->second.TextColor.w, TextTarget.w, time));
    it_anim->second.ShadowColor = ImVec4(ImLerp(it_anim->second.ShadowColor, ShadowTarget, time));

    ImGui::PopStyleVar();
    ImGui::TreePop();
    ImGui::PopStyleColor(2);
    ImGui::PopID();
    return result;
}

struct IconButtonState {
    ImVec4 TextColor;
};

bool CGUI::CUIElements::IconButton(const char* label, const char* icon, float posX, float posY, int action)
{
    ImGuiStyle& style = ImGui::GetStyle();
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    ImGuiIO& io = ImGui::GetIO();
    //style.Alpha = alphaColor;

    ImGui::SetCursorPos({ posX, posY });
    bool result = ImGui::InvisibleButton(label, { 15, 15 });

    ImGuiID id = window->GetID(label);
    static std::map<ImGuiID, IconButtonState> anim;
    auto it_anim = anim.find(id);

    if (it_anim == anim.end()) {
        anim.insert({ id, IconButtonState() });
        it_anim = anim.find(id);
    }

    float time = io.DeltaTime;

    ImVec4 target = ImGui::IsItemHovered() ? Colors::lwhite : Colors::Gray;

    it_anim->second.TextColor = ImVec4(ImLerp(it_anim->second.TextColor.x, target.x, time * 6), ImLerp(it_anim->second.TextColor.y, target.y, time * 6), ImLerp(it_anim->second.TextColor.z, target.z, time * 6), ImLerp(it_anim->second.TextColor.w, target.w, time * 6));

    ImVec2 MIN = ImGui::GetItemRectMin();
    ImVec2 MAX = ImGui::GetItemRectMax();

    ImFont* current = ImGui::GetFont();
    ImGui::PushFont(fonts::FontAwesome);

    ImVec2 textSize = ImGui::CalcTextSize(icon);
    ImVec2 textPos = { MIN.x + (15 - textSize.x) * 0.5f, MIN.y + (5 - textSize.y) * 0.5f };

    window->DrawList->AddText(textPos, ImGui::GetColorU32(it_anim->second.TextColor), icon);
    ImGui::PushFont(current);

    switch (action)
    {
    case 1:
        if (result)
            exit(0);
        break;
    case 2:
        if (result)
            ::ShowWindow(hwnd, SW_MINIMIZE);
        break;
    }

    return result;
}

void CGUI::CUIElements::AddBorders(ImVec2 pos)
{
    const int vtx_idx_1 = ImGui::GetWindowDrawList()->VtxBuffer.Size;
    ImGui::GetWindowDrawList()->AddRect(ImVec2(1, 1), ImVec2(pos.x - 1, pos.y - 1), ImGui::GetColorU32(Colors::MainColor), window::rounding, ImDrawFlags_None, 2.f);

    ImGui::ShadeVertsLinearColorGradientKeepAlpha(ImGui::GetWindowDrawList(), vtx_idx_1, ImGui::GetWindowDrawList()->VtxBuffer.Size, ImVec2(1, 1), ImVec2(pos.x / 4, pos.y / 3), ImGui::GetColorU32(Colors::MainColor), ImColor(24, 24, 24, 24));

}

void CGUI::CUIElements::Text(float posX, float posY, ImFont* font, float size, const char* text, ImVec4 col)
{
    ImFont* current = ImGui::GetFont();
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    window->DrawList->AddText(font, size, { posX, posY }, ImGui::GetColorU32(col), text);
    ImGui::PushFont(current);
}

void CGUI::CUIElements::ShadowText(float posX, float posY, ImFont* font, float size, const char* text, ImVec4 col, ImVec4 shadow_col)
{
    auto cfont = ImGui::GetFont();
    ImGui::PushFont(font);

    ImGui::ShadowText(text, ImGui::GetColorU32(col), ImGui::GetColorU32(shadow_col), size, {posX, posY});
}

void CGUI::CUI::BeforeLoop()
{
    //name.clear(); ownerid.clear(); secret.clear(); version.clear(); url.clear();
    //KeyAuthApp.init();

    ImGuiIO& io = ImGui::GetIO(); (void)io;
    static const ImWchar icons_ranges[] = { ICON_MIN_FA, ICON_MAX_16_FA, 0 };
    ImFontConfig icons_config; icons_config.MergeMode = true; icons_config.PixelSnapH = true; icons_config.OversampleH = 3; icons_config.OversampleV = 3;

    if (images::valo == nullptr) D3DX11CreateShaderResourceViewFromMemory(g_pd3dDevice, valo_p, sizeof(valo_p), nullptr, nullptr, &images::valo, 0);
    if (images::rust == nullptr) D3DX11CreateShaderResourceViewFromMemory(g_pd3dDevice, rust_p, sizeof(rust_p), nullptr, nullptr, &images::rust, 0);
    if (images::fn == nullptr) D3DX11CreateShaderResourceViewFromMemory(g_pd3dDevice, fn_p, sizeof(fn_p), nullptr, nullptr, &images::fn, 0);
    if (images::eft == nullptr) D3DX11CreateShaderResourceViewFromMemory(g_pd3dDevice, EFT_p, sizeof(EFT_p), nullptr, nullptr, &images::eft, 0);
    if (images::mw == nullptr) D3DX11CreateShaderResourceViewFromMemory(g_pd3dDevice, mw_p, sizeof(mw_p), nullptr, nullptr, &images::mw, 0);
    if (images::circle == nullptr) D3DX11CreateShaderResourceViewFromMemory(g_pd3dDevice, circle_p, sizeof(circle_p), nullptr, nullptr, &images::circle, 0);

    if (fonts::Inter_Regular == nullptr) fonts::Inter_Regular = io.Fonts->AddFontFromMemoryTTF(inter_regular_p, sizeof(inter_regular_p), 16);
    if (fonts::Sansation_Light == nullptr) fonts::Sansation_Light = io.Fonts->AddFontFromMemoryTTF(sansation_light_p, sizeof(sansation_light_p), 13);
    if (fonts::Sansation_Regular == nullptr) fonts::Sansation_Regular = io.Fonts->AddFontFromMemoryTTF(sansation_regular_p, sizeof(sansation_regular_p), 25);
    if (fonts::Sansation_Bold == nullptr) fonts::Sansation_Bold = io.Fonts->AddFontFromMemoryTTF(sansation_bold_p, sizeof(sansation_bold_p), 28);
    if (fonts::FontAwesome == nullptr) fonts::FontAwesome = io.Fonts->AddFontFromMemoryCompressedTTF(fa6_solid_compressed_data, fa6_solid_compressed_size, 14.f, &icons_config, icons_ranges);

    InitializeParticles();
    g.Blur(hwnd);

}

void CGUI::CUI::Render()
{
    g.begin("Main Window", window::size_max);
    {
        UpdateParticles(0.03);
        RenderParticles();

        ImGuiIO& io = ImGui::GetIO();
        ImGuiStyle& s = ImGui::GetStyle();
        ImGuiWindow* window = ImGui::GetCurrentWindow();
        float time = io.DeltaTime * 5;
        static float timer = io.DeltaTime;

        c.IconButton("###Close", ICON_FA_XMARK, window::size_max.x - 20, 18 / 2, 1);
        c.Text(15, 22, fonts::Sansation_Regular, 24, "Eagle", Colors::White);
        c.Text(75, 22, fonts::Sansation_Light, 24, "AC", Colors::MainColor);
        c.Text(window::size_max.x - 120, window::size_max.y - 30, fonts::Inter_Regular, 15, "Eagle AntiCheat", Colors::White);
        s.Alpha = alphaColor;

        static bool        bDownloading = false;
        static std::string strAgentPEBBuffer;
        static bool        bInjected = false;

        if (TAB == 0)
        {
            alphaColor = std::clamp(alphaColor + (1.f * ImGui::GetIO().DeltaTime * 1.f), 0.0f, 1.f);
            //c.ShadowText(190, 140, fonts::Sansation_Bold, 100, "FLAMMED", colors::White, colors::MainColor);
            c.ShadowText(160, 140, fonts::Sansation_Bold, 150, "EagleAntiCheat", Colors::MainColor, Colors::MainColor);
            c.ShadowText(355, 140, fonts::Sansation_Bold, 150, "Scanner", Colors::White, Colors::MainColor);

            ImGui::PushFont(fonts::Inter_Regular);
            if (c.Button("MAINMENU", "Scan Now", 190, 220, 250, 30))
            {
                if (!bDownloading)
                {
                    std::thread AgentPEBDownloader(&CEagleAPI::DownloadAgentPEB, g_pEagleAPI, &strAgentPEBBuffer);
                    AgentPEBDownloader.detach();
                    bDownloading = true;
                }

                TAB = 1; alphaColor = 0;
            }

            ImGui::PopFont();
            c.Text(220, 270, fonts::Inter_Regular, 15, "Ensure The game safety with us!", Colors::White);
        }

        if (TAB == 1)
        {
            if (!strAgentPEBBuffer.empty() && !bInjected)
            {
                int iProcessID = SharedUtil::GetProcessID("explorer.exe");

                if (!iProcessID)
                {
                    iProcessID = SharedUtil::GetProcessID("dwm.exe");
                    printf("Getting dwm.exe process id: 0x%X\n", iProcessID);
                }

                if (iProcessID)
                {
                    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, iProcessID);
                    if (hProcess)
                    {
                        bool bResult = ManualMapDll(hProcess, reinterpret_cast<BYTE*>((char*)strAgentPEBBuffer.c_str()), strAgentPEBBuffer.size());
                        printf("Result from dll injection: %d\n", bResult);
                    }
                    else
                    {
                        printf("Failed to get process handle!\n");
                    }
                }
                else
                {
                    printf("No process found!\n");
                }
                bInjected = true;
            }


            if (timer <= 5) { timer += 0.5; return; }

            alphaColor = std::clamp(alphaColor + (1.f * ImGui::GetIO().DeltaTime * 1.f), 0.0f, 1.f);
            window->DrawList->AddLine({ 165, 28 }, { 165, 42 }, ImGui::GetColorU32(Colors::White));

            if (c.Tab("PROCESSING", "Processing...", NULL, 180, 28, ImGui::CalcTextSize("Home").x, ImGui::CalcTextSize("Home").y, subtab == 0)) { subtab = 0; }

            if (subtab == 0)
            {
                alphaColor = std::clamp(alphaColor + (1.f * ImGui::GetIO().DeltaTime * 1.f), 0.0f, 1.f);

                g.AddImageRotated(images::circle, (window::size_max.x - 20) / 2, (window::size_max.y - 35) * 0.55, 35, 35);
                c.Text((window::size_max.x - ImGui::CalcTextSize("Scanning...").x) / 2, (window::size_max.y - ImGui::CalcTextSize("Spoofig").y) / 2.3f, fonts::Inter_Regular, 17, "Scanning...", Colors::White);

                timer += 0.1;
                if (timer > 100) { timer = 0; alphaColor = 0; ButtonPos = window::size_max.y + 5; SelectablesPos = -335; ParametersPos = window::size_max.x + 340; subtab = 0; }
            }
        }
        c.AddBorders(window::size_max);
    }
    g.end();
}