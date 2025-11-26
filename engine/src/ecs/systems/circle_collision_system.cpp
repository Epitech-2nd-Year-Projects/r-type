#include "engine/ecs/systems/circle_collision_system.h"

#include <vector>

#include "engine/ecs/components/circle_collider_component.h"
#include "engine/ecs/components/position_component.h"
#include "engine/ecs/indexed_zipper.h"
#include "engine/math/collision.h"

namespace engine::ecs {

void CircleCollisionSystem::Update(Registry& registry, time::TimeDelta dt) {
  auto& positions = registry.GetComponents<PositionComponent>();
  auto& colliders = registry.GetComponents<CircleColliderComponent>();
  std::vector<std::tuple<EntityId, math::Vector2f, float>> entities;

  for (auto&& [idx, pos, collider] : IndexedZipper(positions, colliders)) {
    EntityId entity = registry.EntityFromIndex(idx);
    math::Vector2f center = pos.value().position + collider.value().offset;
    float radius = collider.value().radius;
    entities.emplace_back(entity, center, radius);
  }
  for (size_t i = 0; i < entities.size(); ++i) {
    for (size_t j = i + 1; j < entities.size(); ++j) {
      const auto& [entity_a, center_a, radius_a] = entities[i];
      const auto& [entity_b, center_b, radius_b] = entities[j];

      if (math::Collision::CircleVsCircle(center_a, radius_a, center_b,
                                          radius_b)) {
        if (on_collision_) {
          on_collision_(entity_a, entity_b);
        }
      }
    }
  }
}

void CircleCollisionSystem::SetCollisionCallback(
    CircleCollisionCallback callback) {
  on_collision_ = std::move(callback);
}

}  // namespace engine::ecs