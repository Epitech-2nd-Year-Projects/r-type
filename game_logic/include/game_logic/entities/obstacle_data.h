#ifndef GAME_LOGIC_ENTITIES_OBSTACLE_DATA_H_
#define GAME_LOGIC_ENTITIES_OBSTACLE_DATA_H_

#include <cstdint>

#include "engine/render/color.h"

namespace game_logic::entities {

/**
 * @struct ObstacleArchetypeData
 * @brief Complete obstacle type definition (immutable)
 */
struct ObstacleArchetypeData {
  const char* name;
  bool destructible;
  std::uint32_t health;
  std::uint32_t score_value;
  const char* texture_path;
  engine::render::Color tint_color;
};

/**
 * @brief Indestructible wall archetype data
 */
inline constexpr ObstacleArchetypeData kWallData = {
    "Wall",
    false,
    0,
    0,
    "assets/sprites/obstacle_wall.png",
    engine::render::Color::FromBytes(128, 128, 128, 255)};

/**
 * @brief Destructible barrier archetype data
 */
inline constexpr ObstacleArchetypeData kDestructibleBarrierData = {
    "DestructibleBarrier",
    true,
    100,
    50,
    "assets/sprites/obstacle_destructible.png",
    engine::render::Color::FromBytes(160, 120, 80, 255)};

}  // namespace game_logic::entities

#endif  // GAME_LOGIC_ENTITIES_OBSTACLE_DATA_H_