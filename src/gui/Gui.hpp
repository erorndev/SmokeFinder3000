#pragma once
#include <d3d11.h>
#include <dxgi.h>
#include <windows.h>

namespace Gui {
void Initialize();
void Render();
void Shutdown();
void SetOriginalWndProc(WNDPROC proc);
bool IsInitialized();
bool IsMenuOpen();
void ToggleMenu();
bool ShouldUnload();
LRESULT WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
HWND GetWindow();
WNDPROC GetOriginalWndProc();
} // namespace Gui