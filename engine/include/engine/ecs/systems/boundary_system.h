#ifndef ENGINE_ECS_SYSTEMS_BOUNDARY_SYSTEM_H_
#define ENGINE_ECS_SYSTEMS_BOUNDARY_SYSTEM_H_

#include "engine/ecs/registry.h"
#include "engine/ecs/system.h"
#include "engine/math/rect.h"
#include "engine/time/time_delta.h"

namespace engine::ecs {

/**
 * @brief Constrains entities within a boundary area
 *
 * @details
 * Prevents entities with PositionComponent from leaving a defined
 * rectangular area. Clamps position to boundary edges.
 */
class BoundarySystem : public ISystem {
 public:
  /**
   * @brief Create boundary system with defined area
   */
  explicit BoundarySystem(const math::RectF& bounds);

  void Update(Registry& registry, time::TimeDelta dt) override;

  /**
   * @brief Set boundary area
   */
  void SetBounds(const math::RectF& bounds);

  /**
   * @brief Get current boundary area
   */
  math::RectF GetBounds() const;

 private:
  math::RectF bounds_;
};

}  // namespace engine::ecs

#endif  // ENGINE_ECS_SYSTEMS_BOUNDARY_SYSTEM_H_
