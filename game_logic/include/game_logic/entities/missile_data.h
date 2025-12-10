#ifndef GAME_LOGIC_ENTITIES_MISSILE_DATA_H_
#define GAME_LOGIC_ENTITIES_MISSILE_DATA_H_

#include <cstdint>
#include <string_view>

#include "engine/render/color.h"

namespace game_logic::entities {

/**
 * @struct MissileArchetypeData
 * @brief Complete projectile type definition (immutable)
 */
struct MissileArchetypeData {
  std::string_view name;
  std::uint32_t damage;
  float fire_rate;
  float lifetime_seconds;
  float sprite_width;
  float sprite_height;
  float hitbox_scale;
  std::string_view texture_path;
  engine::render::Color tint_color;
};

/**
 * @brief Player missile archetype data
 */
inline constexpr MissileArchetypeData kPlayerMissileData = {
    "PlayerMissile",
    10,
    2.0f,
    5.0f,
    16.0f,
    8.0f,
    0.8f,
    "assets/sprites/player_missile.png",
    engine::render::Color::FromBytes(100, 150, 255, 255)};

/**
 * @brief Big Player missile archetype data
 */
inline constexpr MissileArchetypeData kBigPlayerMissileData = {
    "BigPlayerMissile",
    50,
    0.5f,
    5.0f,
    32.0f,
    16.0f,
    0.8f,
    "assets/sprites/big_missile.png",
    engine::render::Color::FromBytes(255, 50, 50, 255)};

/**
 * @brief Enemy missile archetype data
 */
inline constexpr MissileArchetypeData kEnemyMissileData = {
    "EnemyMissile",
    50,
    1.0f,
    5.0f,
    12.0f,
    12.0f,
    0.8f,
    "assets/sprites/enemy_missile.png",
    engine::render::Color::FromBytes(255, 100, 100, 255)};

/**
 * @brief Neutral/environment projectile archetype data
 */
inline constexpr MissileArchetypeData kNeutralMissileData = {
    "NeutralMissile",
    20,
    1.0f,
    5.0f,
    14.0f,
    10.0f,
    0.8f,
    "assets/sprites/neutral_missile.png",
    engine::render::Color::FromBytes(200, 200, 200, 255)};

}  // namespace game_logic::entities

#endif  // GAME_LOGIC_ENTITIES_MISSILE_DATA_H_
