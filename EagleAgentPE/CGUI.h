#pragma once
#include "Main.h"
#include "Settings.h"
#include "SharedUtil.h"

#include <algorithm>

class CGUI
{
public:
    class CUI
    {
    public:
        void Render();
        void BeforeLoop();
    };

    class GUI
    {
    public:
        void begin(const char* name, ImVec2 size);
        void end();
        void ChildBegin(const char* name, float posX, float posY, float sizeX, float sizeY);
        void ChildEnd();
        void mw();
        void style();
        void Blur(HWND hwnd);
        void AddImage(ID3D11ShaderResourceView* pic, float posX, float posY, float sizeX, float sizeY);
        void AddImageRotated(ID3D11ShaderResourceView* pic, float posX, float posY, float sizeX, float sizeY);
        void pf(ImFont* font);
        void ApplyImage(int i);
    };

    class CUIElements
    {
    public:
        bool Button(const char* label, const char* text, float posX, float posY, float sizeX, float sizeY);
        bool Tab(const char* label, const char* text, const char* icon, float posX, float posY, float sizeX, float sizeY, bool v);
        bool SelectableItem(const char* label, ID3D11ShaderResourceView* pic, float posX, float posY, float sizeX, float sizeY, bool v);
        bool Checkbox2(const char* label, const char* text, float posX, float posY, bool* v, float space);
        bool InputText(const char* label, const char* hint, float posX, float posY, float sizeX, static char buf[], size_t buf_size, ImGuiInputTextFlags flag);

        void AddBorders(ImVec2 pos);
        bool IconButton(const char* label, const char* icon, float posX, float posY, int action);
        void Text(float posX, float posY, ImFont* font, float size, const char* text, const ImVec4 col);
        void ShadowText(float posX, float posY, ImFont* fofnt, float size, const char* text, ImVec4 col, ImVec4 shadow_col);
    };
};
