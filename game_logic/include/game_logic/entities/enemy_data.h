#ifndef GAME_LOGIC_ENTITIES_ENEMY_DATA_H_
#define GAME_LOGIC_ENTITIES_ENEMY_DATA_H_

#include <cstdint>

#include "game_logic/components/ai_component.h"

namespace game_logic::entities {

/**
 * @struct EnemyArchetypeData
 * @brief Complete enemy type definition (immutable)
 */
struct EnemyArchetypeData {
  const char* name;
  std::uint32_t health;
  float speed;
  components::EnemyBehavior behavior;
  std::uint32_t score;
  float sprite_width;
  float sprite_height;
  float hitbox_width;
  float hitbox_height;
  const char* texture_path;
  float wave_amplitude;
  float wave_frequency;
};

inline constexpr EnemyArchetypeData kScoutData = {
    "Scout", 30,    150.0f, components::EnemyBehavior::kStraight, 100,  24.0f,
    24.0f,   20.0f, 20.0f,  "assets/sprites/enemy_scout.png",     0.0f, 0.0f};

inline constexpr EnemyArchetypeData kBomberData = {
    "Bomber", 60,
    100.0f,   components::EnemyBehavior::kWavePattern,
    200,      32.0f,
    32.0f,    28.0f,
    28.0f,    "assets/sprites/enemy_bomber.png",
    50.0f,    2.0f};

inline constexpr EnemyArchetypeData kTankData = {
    "Tank", 150,   50.0f, components::EnemyBehavior::kPatrol, 500,  48.0f,
    48.0f,  44.0f, 44.0f, "assets/sprites/enemy_tank.png",    0.0f, 0.0f};

}  // namespace game_logic::entities

#endif  // GAME_LOGIC_ENTITIES_ENEMY_DATA_H_