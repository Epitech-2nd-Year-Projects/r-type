/**
 * @file player_prediction_system.h
 * @brief Predicts local player render position between snapshots
 */

#ifndef CLIENT_ECS_PLAYER_PREDICTION_SYSTEM_H_
#define CLIENT_ECS_PLAYER_PREDICTION_SYSTEM_H_

#include <cstdint>
#include <optional>

#include "ecs/components.h"
#include "engine/ecs/registry.h"
#include "engine/time/time_delta.h"
#include "input/input_layer.h"

namespace client::ecs {

/**
 * @class PlayerPredictionSystem
 * @brief Applies client-side prediction for the local player
 */
class PlayerPredictionSystem {
 public:
  explicit PlayerPredictionSystem(engine::ecs::Registry& registry);

  /**
   * @brief Update the local player render position using input prediction
   * @param dt Frame delta time
   * @param action_state Current input state
   */
  void Update(engine::time::TimeDelta dt, const ActionState& action_state);

 private:
  void RegisterComponents();
  std::optional<engine::ecs::EntityId> FindLocalPlayer() const;
  engine::math::Vector2f BuildVelocity(const ActionState& action_state) const;

  engine::ecs::Registry& registry_;
  std::optional<engine::ecs::EntityId> last_local_entity_;
  std::uint32_t last_snapshot_id_{0};
  engine::math::Vector2f predicted_position_{0.0f, 0.0f};
  bool has_prediction_{false};
  float correction_rate_{10.0f};
  float snap_distance_{48.0f};
};

}  // namespace client::ecs

#endif  // CLIENT_ECS_PLAYER_PREDICTION_SYSTEM_H_
