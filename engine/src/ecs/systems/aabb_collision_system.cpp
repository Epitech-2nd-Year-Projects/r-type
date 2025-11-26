#include "engine/ecs/systems/aabb_collision_system.h"

#include <vector>

#include "engine/ecs/components/bounding_box_component.h"
#include "engine/ecs/components/position_component.h"
#include "engine/ecs/indexed_zipper.h"
#include "engine/math/collision.h"

namespace engine::ecs {

void AABBCollisionSystem::Update(Registry& registry, time::TimeDelta dt) {
  auto& positions = registry.GetComponents<PositionComponent>();
  auto& boxes = registry.GetComponents<BoundingBoxComponent>();
  std::vector<std::tuple<EntityId, math::RectF>> entities;

  for (auto&& [idx, pos, box] : IndexedZipper(positions, boxes)) {
    EntityId entity = registry.EntityFromIndex(idx);
    math::RectF world_box = box.value().bounds;
    world_box.Translate(pos.value().position);
    entities.emplace_back(entity, world_box);
  }

  for (size_t i = 0; i < entities.size(); ++i) {
    for (size_t j = i + 1; j < entities.size(); ++j) {
      const auto& [entity_a, box_a] = entities[i];
      const auto& [entity_b, box_b] = entities[j];
      math::CollisionInfo info = math::Collision::AABBCollision(box_a, box_b);
      if (info.colliding_ && on_collision_) {
        on_collision_(entity_a, entity_b, info);
      }
    }
  }
}
void AABBCollisionSystem::SetCollisionCallback(CollisionCallback callback) {
  on_collision_ = std::move(callback);
}

}  // namespace engine::ecs