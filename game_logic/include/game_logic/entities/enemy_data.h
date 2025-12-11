#ifndef GAME_LOGIC_ENTITIES_ENEMY_DATA_H_
#define GAME_LOGIC_ENTITIES_ENEMY_DATA_H_

#include <cstdint>
#include <string_view>

#include "game_logic/components/ai_component.h"

namespace game_logic::entities {

/**
 * @struct EnemyArchetypeData
 * @brief Complete enemy type definition (immutable)
 */
struct EnemyArchetypeData {
  std::string_view name;
  std::uint32_t health;
  float speed;
  components::EnemyBehavior behavior;
  std::uint32_t score;
  float sprite_width;
  float sprite_height;
  float hitbox_width;
  float hitbox_height;
  std::string_view texture_path;
  float wave_amplitude;
  float wave_frequency;
  float detection_range;
  bool can_shoot;
  float fire_rate;
};

inline constexpr EnemyArchetypeData kScoutData = {
    "Scout", 10,    150.0f, components::EnemyBehavior::kStraight, 100,  33.0f,
    33.0f,   33.0f, 33.0f,  "assets/sprites/enemy_scout.png",     0.0f, 0.0f,
    0.0f,    true,  0.5f};

inline constexpr EnemyArchetypeData kBomberData = {
    "Bomber", 20,
    100.0f,   components::EnemyBehavior::kWavePattern,
    200,      33.0f,
    33.0f,    33.0f,
    33.0f,    "assets/sprites/enemy_bomber.png",
    50.0f,    2.0f,
    0.0f,     false,
    0.0f};

inline constexpr EnemyArchetypeData kTankData = {
    "Tank", 150,   50.0f, components::EnemyBehavior::kPatrol, 500,  33.0f,
    33.0f,  33.0f, 33.0f, "assets/sprites/enemy_tank.png",    0.0f, 0.0f,
    0.0f,   false, 0.0f};

inline constexpr EnemyArchetypeData kInterceptorData = {
    "Interceptor", 10,
    150.0f,        components::EnemyBehavior::kChasePlayer,
    300,           33.0f,
    33.0f,         33.0f,
    33.0f,         "assets/sprites/enemy_interceptor.png",
    0.0f,          0.0f,
    1000.0f,       false,
    0.0f};

}  // namespace game_logic::entities

#endif  // GAME_LOGIC_ENTITIES_ENEMY_DATA_H_
