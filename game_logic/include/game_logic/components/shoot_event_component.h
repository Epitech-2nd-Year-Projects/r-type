#ifndef GAME_LOGIC_COMPONENTS_SHOOT_EVENT_COMPONENT_H_
#define GAME_LOGIC_COMPONENTS_SHOOT_EVENT_COMPONENT_H_

#include "engine/math/vector2.h"
#include <cstdint>

namespace game_logic::components {

/**
 * @brief Component attached to a player to trigger a shoot event with lag compensation.
 * 
 * If 'fired' is true, the weapon system will spawn a projectile.
 * 'spawn_position' is where the player WAS when they pressed the button (server-rewound).
 * 'latency_s' is how much time has passed since the shot (to fast-forward the projectile).
 */
struct ShootEventComponent {
  bool fired{false};
  bool is_big_shot{false};
  engine::math::Vector2f spawn_position{0.0f, 0.0f};
  float latency_s{0.0f};
};

}  // namespace game_logic::components

#endif // GAME_LOGIC_COMPONENTS_SHOOT_EVENT_COMPONENT_H_
