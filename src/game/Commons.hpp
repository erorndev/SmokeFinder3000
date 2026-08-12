#pragma once
#include <cstdint>

namespace Commons {
constexpr uint32_t m_pGameSceneNode    = 0x330;
constexpr uint32_t m_iHealth           = 0x34C;
constexpr uint32_t m_lifeState         = 0x354;
constexpr uint32_t m_vecAbsOrigin      = 0xC8;
constexpr uint32_t m_hPlayerPawn       = 0x914;
constexpr uint32_t m_bPawnIsAlive      = 0x91C;
constexpr uint32_t m_pMovementServices = 0x1248;
constexpr uint32_t m_angEyeAngles      = 0x3350;

namespace Interfaces {
    constexpr const char* CvarModule = "tier0.dll";
    constexpr const char* Cvar       = "VEngineCvar007";
} // namespace Interfaces

namespace Signatures {
    constexpr const char* ClientModule = "client.dll";
    constexpr const char* GetLocalPawn =
        "48 83 EC ? 83 F9 ? 75 ? 48 8B 0D ? ? ? ? 48 8D 54 24 ? ? ? ? FF 90 "
        "? ? ? ? ? ? 48 63 C1 4C 8D 05";
} // namespace Signatures
} // namespace Commons