#pragma once
#include <cstdint>

namespace Commons {
  constexpr uint32_t m_pGameSceneNode = 0x330;
  constexpr uint32_t m_iHealth = 0x34C;
  constexpr uint32_t m_lifeState = 0x354;
  constexpr uint32_t m_vecAbsOrigin = 0xC8;
  constexpr uint32_t m_hPlayerPawn = 0x914;
  constexpr uint32_t m_bPawnIsAlive = 0x91C;
  constexpr uint32_t m_pMovementServices = 0x1248;
  constexpr uint32_t m_angEyeAngles = 0x3350;
  constexpr std::ptrdiff_t VEngineCvar007 = 0x3A44F0;
}  // namespace Schema