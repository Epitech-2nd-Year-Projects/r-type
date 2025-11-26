#ifndef ENGINE_ECS_SYSTEMS_LIFETIME_SYSTEM_H_
#define ENGINE_ECS_SYSTEMS_LIFETIME_SYSTEM_H_

#include "engine/ecs/components/lifetime_component.h"
#include "engine/ecs/registry.h"
#include "engine/ecs/system.h"
#include "engine/time/time_delta.h"

namespace engine::ecs {

/**
 * @brief Decrements lifetime and destroys expired entities
 *
 * @details
 * Iterates over entities with LifetimeComponent, decrements remaining
 * time by delta, and kills entities when lifetime reaches zero.
 *
 * Run as Variable system to match actual frame time.
 */
class LifetimeSystem {
 public:
  static void Update(Registry& registry,
                     SparseArray<LifetimeComponent>& lifetimes,
                     time::TimeDelta dt);
};

}  // namespace engine::ecs

#endif  // ENGINE_ECS_SYSTEMS_LIFETIME_SYSTEM_H_