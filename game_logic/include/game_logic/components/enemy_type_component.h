#ifndef GAME_LOGIC_COMPONENTS_ENEMY_TYPE_COMPONENT_H_
#define GAME_LOGIC_COMPONENTS_ENEMY_TYPE_COMPONENT_H_

#include <cstdint>

namespace game_logic::components {

struct EnemyTypeComponent {
  std::uint16_t type_code{0};
};

}  // namespace game_logic::components

#endif  // GAME_LOGIC_COMPONENTS_ENEMY_TYPE_COMPONENT_H_
