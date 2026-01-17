#ifndef GAME_LOGIC_CONSTANTS_H_
#define GAME_LOGIC_CONSTANTS_H_

#include <cstdint>

namespace game_logic {

// Grid Configuration
constexpr float kGridCellSize = 100.0f;

// World Dimensions
constexpr float kGameWidth = 1600.0f;
constexpr float kGameHeight = 880.0f;

// Player Spawn Configuration
constexpr float kPlayerSpawnBaseX = 100.0f;
constexpr float kPlayerSpawnOffsetX = 50.0f;
constexpr float kPlayerSpawnY = 300.0f;
constexpr float kSpawnMinY = 50.0f;
constexpr float kSpawnMaxY = 700.0f;

// Enemy Visuals
constexpr std::uint8_t kEnemyTintR = 255;
constexpr std::uint8_t kEnemyTintG = 80;
constexpr std::uint8_t kEnemyTintB = 80;
constexpr std::uint8_t kEnemyTintA = 255;
constexpr int kEnemyLayer = 5;

// Enemy Behavior
constexpr float kPatrolMinX = 0.0f;
constexpr float kPatrolMinY = 100.0f;
constexpr float kPatrolMaxX = 800.0f;
constexpr float kPatrolMaxY = 500.0f;

// Collision
constexpr std::uint32_t kCrashDamage = 100;

// Cleanup
constexpr float kOffscreenCleanupBoundary = -200.0f;
constexpr float kPowerupCleanupBoundary = -100.0f;

// Powerups
constexpr float kPowerupDriftSpeed = -20.0f;

// Game Logic
constexpr float kLevelTransitionDelay = 3.0f;

}  // namespace game_logic

#endif  // GAME_LOGIC_CONSTANTS_H_
