#ifndef GAME_LOGIC_ENTITIES_POWERUP_DATA_H_
#define GAME_LOGIC_ENTITIES_POWERUP_DATA_H_

#include <cstdint>
#include <string_view>

#include "game_logic/constants.h"

namespace game_logic::entities {

struct PowerupArchetypeData {
  std::string_view name;
  float sprite_width;
  float sprite_height;
  std::string_view texture_path;
};

inline constexpr PowerupArchetypeData kHealthPotionData = {
    "HealthPotion", 16.0f, 16.0f, "assets/sprites/powerup_green.png"};

}  // namespace game_logic::entities

#endif  // GAME_LOGIC_ENTITIES_POWERUP_DATA_H_
