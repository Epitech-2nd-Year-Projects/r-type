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
  float speed;
};

/**
 * @brief Player missile archetype data
 */
inline constexpr MissileArchetypeData kPlayerMissileData = {
    "PlayerMissile",
    10,
    2.0f,
    5.0f,
    19.0f,
    6.0f,
    0.8f,
    "assets/sprites/player_missile.png",
    engine::render::Color::FromBytes(255, 255, 255, 255),
    300.0f};

/**
 * @brief Big Player missile archetype data
 */
inline constexpr MissileArchetypeData kBigPlayerMissileData = {
    "BigPlayerMissile",
    50,
    0.5f,
    5.0f,
    19.0f,
    6.0f,
    0.8f,
    "assets/sprites/big_missile.png",
    engine::render::Color::FromBytes(255, 255, 255, 255),
    250.0f};

/**
 * @brief Enemy missile archetype data
 */
inline constexpr MissileArchetypeData kEnemyMissileData = {
    "EnemyMissile",
    50,
    1.0f,
    5.0f,
    19.0f,
    6.0f,
    0.8f,
    "assets/sprites/enemy_missile.png",
    engine::render::Color::FromBytes(255, 255, 255, 255),
    300.0f};

/**
 * @brief Neutral/environment projectile archetype data
 */
inline constexpr MissileArchetypeData kNeutralMissileData = {
    "NeutralMissile",
    20,
    1.0f,
    5.0f,
    19.0f,
    6.0f,
    0.8f,
    "assets/sprites/neutral_missile.png",
    engine::render::Color::FromBytes(255, 255, 255, 255),
    150.0f};

}  // namespace game_logic::entities

#endif  // GAME_LOGIC_ENTITIES_MISSILE_DATA_H_
