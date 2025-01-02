#include "StdInc.h"
#include "Main.h"
#include "GUI/GUI.h"

int main(int argc, char* argv[])
{
    if (GUI::Initialize())
    {
        GUI::RenderUI();
    }
    GUI::Destroy();

    return 0;
}