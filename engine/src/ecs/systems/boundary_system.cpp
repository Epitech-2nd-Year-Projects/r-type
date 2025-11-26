#include "engine/ecs/systems/boundary_system.h"

#include <algorithm>

#include "engine/ecs/components/position_component.h"
#include "engine/ecs/zipper.h"

namespace engine::ecs {

BoundarySystem::BoundarySystem(const math::RectF& bounds) : bounds_(bounds) {}

void BoundarySystem::Update(Registry& registry, time::TimeDelta dt) {
  auto& positions = registry.GetComponents<PositionComponent>();
  for (auto&& [pos] : Zipper(positions)) {
    auto& position = pos.value().position;
    position.x = std::clamp(position.x, bounds_.top_left_x_,
                            bounds_.top_left_x_ + bounds_.width_);
    position.y = std::clamp(position.y, bounds_.top_left_y_,
                            bounds_.top_left_y_ + bounds_.height_);
  }
}

void BoundarySystem::SetBounds(const math::RectF& bounds) { bounds_ = bounds; }

math::RectF BoundarySystem::GetBounds() const { return bounds_; }

}  // namespace engine::ecs