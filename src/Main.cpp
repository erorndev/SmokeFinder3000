#include <windows.h>

#include "gui/Gui.hpp"

DWORD WINAPI MainThread(LPVOID lpParam) {
  Gui::Initialize();

  while (true) {
    if (GetAsyncKeyState(VK_INSERT) & 1) {
      Gui::ToggleMenu();
    }
    Sleep(100);
  }

  Gui::Shutdown();
  FreeLibraryAndExitThread((HMODULE)lpParam, 0);
  return 0;
}