#ifndef GAME_LOGIC_SYSTEMS_BOUNDARY_SYSTEM_H_
#define GAME_LOGIC_SYSTEMS_BOUNDARY_SYSTEM_H_

#include "engine/ecs/system.h"
#include "engine/time/time_delta.h"

namespace game_logic::systems {

/**
 * @class BoundarySystem
 * @brief Handles entity screen bounds and cleanup.
 *
 * @details
 * This system is responsible for:
 * 1. Constraining player entities within the viewable screen area.
 * 2. Removing entities that have scrolled off-screen (e.g. x < -200).
 */
class BoundarySystem : public engine::ecs::ISystem {
 public:
  /**
   * @brief Construct a new Boundary System
   *
   * @param screen_width Width of the game screen (default 1920)
   * @param screen_height Height of the game screen (default 1080)
   */
  BoundarySystem(float screen_width = 1920.0f, float screen_height = 1080.0f);

  ~BoundarySystem() override = default;

  /**
   * @brief Update entities for boundary constraints and cleanup.
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

#endif  // GAME_LOGIC_SYSTEMS_BOUNDARY_SYSTEM_H_
