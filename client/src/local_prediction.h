/**
 * @file local_prediction.h
 * @brief Client-side prediction for the locally controlled player
 *
 * Tracks the local player entity, applies input-driven movement every frame
 * and smooths corrections when authoritative snapshots arrive to mask network
 * latency.
 */

#ifndef CLIENT_LOCAL_PREDICTION_H_
#define CLIENT_LOCAL_PREDICTION_H_

#include <optional>

#include "ecs/components.h"
#include "engine/ecs/registry.h"
#include "engine/math/vector2.h"
#include "engine/time/time_delta.h"
#include "input/input_buffer.h"
#include "join_flow.h"

namespace client {

/**
 * @class LocalPrediction
 * @brief Predicts local player movement and reconciles with server snapshots
 */
class LocalPrediction {
 public:
  LocalPrediction(engine::ecs::Registry& registry, JoinFlow& join_flow);

  /**
   * @brief Clear local state after disconnect or reset
   */
  void Reset();

  /**
   * @brief Apply current input to the local player for instant feedback
   * @param dt Frame delta time
   * @param input_state Current action state sampled from the input layer
   */
  void Update(engine::time::TimeDelta dt, const ActionState& input_state);

  /**
   * @brief Snapshot the predicted position before applying a server update
   * @return Predicted position of the local player or std::nullopt when unknown
   */
  std::optional<engine::math::Vector2f> CapturePredictedPosition();

  /**
   * @brief Reconcile prediction after an authoritative snapshot
   * @param predicted_before Position used for prediction prior to the snapshot
   */
  void OnSnapshotApplied(
      const std::optional<engine::math::Vector2f>& predicted_before);

 private:
  std::optional<engine::ecs::EntityId> ResolveLocalEntity();
  static engine::math::Vector2f ComputeVelocity(
      const ActionState& input_state);
  static void ClampPosition(engine::math::Vector2f& position);
  void ApplyReconciliation(ecs::PositionComponent& position, float dt_seconds);
  void MarkLocalPlayer(engine::ecs::EntityId entity);

  engine::ecs::Registry& registry_;
  JoinFlow& join_flow_;
  std::optional<engine::ecs::EntityId> local_entity_;
  engine::math::Vector2f reconciliation_offset_{0.0f, 0.0f};
  float reconciliation_elapsed_{0.0f};
  static constexpr float kMoveSpeed = 200.0f;
  static constexpr float kReconciliationDuration = 0.1f;
  static constexpr float kWorldWidth = 1920.0f;
  static constexpr float kWorldHeight = 1080.0f;
};

}  // namespace client

#endif  // CLIENT_LOCAL_PREDICTION_H_
