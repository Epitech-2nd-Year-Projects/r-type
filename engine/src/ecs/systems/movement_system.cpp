#include "engine/ecs/systems/movement_system.h"

#include "engine/ecs/components/position_component.h"
#include "engine/ecs/components/velocity_component.h"
#include "engine/ecs/zipper.h"

namespace engine::ecs {

void MovementSystem::Update(Registry& registry,
                            SparseArray<PositionComponent>& positions,
                            SparseArray<VelocityComponent>& velocities,
                            time::TimeDelta dt) {
  float dt_seconds = dt.as_seconds();

  for (auto&& [pos, vel] : Zipper(positions, velocities)) {
    pos.value().position.x += vel.value().velocity.x * dt_seconds;
    pos.value().position.y += vel.value().velocity.y * dt_seconds;
  }
}

}  // namespace engine::ecs