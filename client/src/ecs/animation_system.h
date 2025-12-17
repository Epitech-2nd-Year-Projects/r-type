#ifndef CLIENT_ECS_ANIMATION_SYSTEM_H_
#define CLIENT_ECS_ANIMATION_SYSTEM_H_

#include "engine/ecs/registry.h"
#include "engine/time/time_delta.h"

namespace client::ecs {

/**
 * @class AnimationSystem
 * @brief Updates sprite animation frames on the client.
 */
class AnimationSystem {
 public:
  explicit AnimationSystem(engine::ecs::Registry& registry);

  void Update(engine::time::TimeDelta dt);

 private:
  engine::ecs::Registry& registry_;
};

}  // namespace client::ecs

#endif  // CLIENT_ECS_ANIMATION_SYSTEM_H_
