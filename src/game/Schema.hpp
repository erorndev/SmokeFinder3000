#pragma once
#include <cstdint>

namespace Schema {
// --- C_BaseEntity ---
constexpr uint32_t m_pGameSceneNode = 0x330;
constexpr uint32_t m_iHealth = 0x34C;
constexpr uint32_t m_lifeState = 0x354;

// --- CGameSceneNode ---
constexpr uint32_t m_vecAbsOrigin = 0xC8;

// --- CCSPlayerController ---
constexpr uint32_t m_hPlayerPawn = 0x914;
constexpr uint32_t m_bPawnIsAlive = 0x91C;

// --- C_BasePlayerPawn ---
constexpr uint32_t m_pMovementServices = 0x1248;
constexpr uint32_t m_angEyeAngles = 0x3350;
}  // namespace Schema