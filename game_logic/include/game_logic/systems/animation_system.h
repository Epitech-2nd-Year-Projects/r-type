#ifndef GAME_LOGIC_SYSTEMS_ANIMATION_SYSTEM_H_
#define GAME_LOGIC_SYSTEMS_ANIMATION_SYSTEM_H_

#include "engine/ecs/registry.h"
#include "engine/ecs/system.h"
#include "engine/time/time_delta.h"

namespace game_logic::systems {

/**
 * @class AnimationSystem
 * @brief Updates sprite animation frames based on elapsed time.
 *
 * @details
 * Iterates over entities with both AnimationComponent and SpriteComponent.
 * Updates the SpriteComponent's source_rect to match the current frame
 * of the animation. Handles looping and one-shot animations.
 */
class AnimationSystem : public engine::ecs::ISystem {
 public:
  AnimationSystem() = default;
  ~AnimationSystem() override = default;

  /**
   * @brief Update animations.
   * @param registry ECS registry
   * @param dt Time delta since last frame
   */
  void Update(engine::ecs::Registry &registry,
              engine::time::TimeDelta dt) override;
};

}  // namespace game_logic::systems

#endif  // GAME_LOGIC_SYSTEMS_ANIMATION_SYSTEM_H_
