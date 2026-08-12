#include "Game.hpp"

#include <windows.h>

#include "Commons.hpp"
#include "Interfaces.hpp"
#include "PatternScan.hpp"

namespace Game {
using GetLocalPawnFn = void* (*)();

static void* g_pCVar                 = nullptr;
static GetLocalPawnFn g_GetLocalPawn = nullptr;

bool Initialize() {
    g_pCVar = Interfaces::Get(Commons::Interfaces::CvarModule, Commons::Interfaces::Cvar);
    if (!g_pCVar) {
        OutputDebugStringA("[SmokeFinder3000] Failed to resolve VEngineCvar007\n");
    }

    g_GetLocalPawn = reinterpret_cast<GetLocalPawnFn>(
        PatternScan::Find(Commons::Signatures::ClientModule, Commons::Signatures::GetLocalPawn));
    if (!g_GetLocalPawn) {
        OutputDebugStringA("[SmokeFinder3000] Failed to resolve GetLocalPawn\n");
    }

    return g_pCVar && g_GetLocalPawn;
}

void* GetCVar() {
    return g_pCVar;
}

void* GetLocalPlayerPawn() {
    return g_GetLocalPawn ? g_GetLocalPawn() : nullptr;
}
} // namespace Game
