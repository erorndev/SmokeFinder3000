#include <windows.h>

#include <algorithm>
#include <cctype>
#include <string>

extern DWORD WINAPI MainThread(LPVOID lpParam);

static bool IsCS2Process() {
    wchar_t path[MAX_PATH] = {0};
    if (GetModuleFileNameW(NULL, path, MAX_PATH) == 0) {
        return false;
    }

    std::wstring exePath(path);
    size_t lastSlash = exePath.find_last_of(L"\\/");
    std::wstring exeName =
        (lastSlash != std::wstring::npos) ? exePath.substr(lastSlash + 1) : exePath;

    std::transform(exeName.begin(), exeName.end(), exeName.begin(), [](wchar_t c) {
        return static_cast<wchar_t>(std::tolower(c));
    });

    return (exeName == L"cs2.exe");
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID reserved) {
    switch (reason) {
    case DLL_PROCESS_ATTACH: {
        DisableThreadLibraryCalls(hModule);
        if (!IsCS2Process()) {
            return FALSE;
        }
        HANDLE hThread = CreateThread(NULL, 0, MainThread, hModule, 0, NULL);
        if (hThread)
            CloseHandle(hThread);
        break;
    }

    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}