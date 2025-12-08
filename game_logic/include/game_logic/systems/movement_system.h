#ifndef GAME_LOGIC_SYSTEMS_MOVEMENT_SYSTEM_H_
#define GAME_LOGIC_SYSTEMS_MOVEMENT_SYSTEM_H_

#include "engine/ecs/system.h"
#include "engine/time/time_delta.h"

namespace game_logic::systems {

/**
 * @class MovementSystem
 * @brief Handles entity movement, velocity application, and screen bounds.
 *
 * @details
 * This system is responsible for:
 * 1. Updating Position based on Velocity * DeltaTime.
 * 2. Constraining player entities within the viewable screen area.
 * 3. Moving other entities (enemies, projectiles) according to R-Type's
 * scrolling mechanics.
 */
class MovementSystem : public engine::ecs::ISystem {
 public:
  /**
   * @brief Construct a new Movement System
   *
   * @param screen_width Width of the game screen (default 1920)
   * @param screen_height Height of the game screen (default 1080)
   */
  MovementSystem(float screen_width = 1920.0f, float screen_height = 1080.0f);

  ~MovementSystem() override = default;

  /**
   * @brief Update all entities with Position and Velocity components.
   *
   * @param registry Reference to the ECS registry.
   * @param dt Time delta since the last frame.
   */
  void Update(engine::ecs::Registry& registry,
              engine::time::TimeDelta dt) override;

 private:
  float screen_width_;
  float screen_height_;
};

}  // namespace game_logic::systems

#endif  // GAME_LOGIC_SYSTEMS_MOVEMENT_SYSTEM_H_
