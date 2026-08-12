#include <windows.h>

#include "game/Game.hpp"
#include "gui/Gui.hpp"

DWORD WINAPI MainThread(LPVOID lpParam) {
    Game::Initialize();
    Gui::Initialize();

    while (true) {
        if (GetAsyncKeyState(VK_INSERT) & 1) {
            Gui::ToggleMenu();
        }

        if (Gui::ShouldUnload()) {
            break;
        }

        Sleep(100);
    }

    Gui::Shutdown();
    FreeLibraryAndExitThread((HMODULE)lpParam, 0);
    return 0;
}