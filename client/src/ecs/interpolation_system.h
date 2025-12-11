/**
 * @file interpolation_system.h
 * @brief Smooths networked entities between snapshots
 */

#ifndef CLIENT_ECS_INTERPOLATION_SYSTEM_H_
#define CLIENT_ECS_INTERPOLATION_SYSTEM_H_

#include <cstdint>

#include "ecs/components.h"
#include "engine/ecs/registry.h"
#include "engine/time/time_delta.h"

namespace client::ecs {

/**
 * @class InterpolationSystem
 * @brief Computes render positions for remote entities from timestamped snapshots
 *
 * @details
 * Uses the timestamps of the last two snapshots applied to an entity and the
 * local monotonic clock to interpolate toward the latest authoritative state
 * while allowing bounded extrapolation when new updates are delayed.
 */
class InterpolationSystem {
 public:
  explicit InterpolationSystem(engine::ecs::Registry& registry);

  /**
   * @brief Update render positions using the latest snapshot timing data
   * @param dt Frame delta time
   */
  void Update(engine::time::TimeDelta dt);

  /**
   * @brief Update using an explicit timestamp useful for testing
   * @param dt Frame delta time
   * @param now_ms Monotonic timestamp in milliseconds
   */
  void UpdateAt(engine::time::TimeDelta dt, std::uint64_t now_ms);

  /**
   * @brief Configure how far the render timeline lags behind the newest snapshot
   * @param delay_ms Interpolation delay in milliseconds
   */
  void SetInterpolationDelayMs(std::uint32_t delay_ms) {
    interpolation_delay_ms_ = delay_ms;
  }

  /**
   * @brief Configure how much extrapolation to allow after the newest snapshot
   * @param max_ms Maximum extrapolation window in milliseconds
   */
  void SetMaxExtrapolationMs(std::uint32_t max_ms) {
    max_extrapolation_ms_ = max_ms;
  }

 private:
  void RegisterComponents();
  static bool HasLocalPlayerTag(const engine::ecs::Registry& registry,
                                engine::ecs::EntityId entity);

  engine::ecs::Registry& registry_;
  std::uint32_t interpolation_delay_ms_{100};
  std::uint32_t max_extrapolation_ms_{150};
};

}  // namespace client::ecs

#endif  // CLIENT_ECS_INTERPOLATION_SYSTEM_H_
