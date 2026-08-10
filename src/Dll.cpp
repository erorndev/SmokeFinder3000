#include <windows.h>

extern DWORD WINAPI MainThread(LPVOID lpParam);

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID reserved) {
  switch (reason) {
    case DLL_PROCESS_ATTACH: {
      DisableThreadLibraryCalls(hModule);
      HANDLE hThread = CreateThread(NULL, 0, MainThread, hModule, 0, NULL);
      if (hThread) CloseHandle(hThread);
      break;
    }

    case DLL_PROCESS_DETACH:
      break;
  }
  return TRUE;
}