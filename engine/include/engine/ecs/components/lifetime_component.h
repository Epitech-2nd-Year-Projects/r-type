#ifndef ENGINE_ECS_COMPONENTS_LIFETIME_COMPONENT_H_
#define ENGINE_ECS_COMPONENTS_LIFETIME_COMPONENT_H_

#include "engine/time/time_delta.h"

namespace engine::ecs {

/**
 * @brief Automatic entity destruction after time expires
 *
 * @details
 * Useful for temporary entities like particles, projectiles, effects.
 * LifetimeSystem decrements remaining time and destroys expired entities.
 */
struct LifetimeComponent {
  time::TimeDelta remaining;

  LifetimeComponent() : remaining(time::TimeDelta::zero()) {}
  explicit LifetimeComponent(time::TimeDelta lifetime) : remaining(lifetime) {}
};

}  // namespace engine::ecs

#endif  // ENGINE_ECS_COMPONENTS_LIFETIME_COMPONENT_H_
