#include "game_logic/systems/collision_system.h"

#include <algorithm>
#include <cmath>
#include <unordered_set>

#include "engine/ecs/components/bounding_box_component.h"
#include "engine/ecs/components/compound_circle_collider_component.h"
#include "engine/ecs/components/position_component.h"
#include "engine/ecs/components/tag_component.h"
#include "engine/ecs/indexed_zipper.h"
#include "engine/ecs/registry.h"
#include "engine/event.h"
#include "engine/math/collision.h"
#include "game_logic/components/damageable_component.h"
#include "game_logic/components/health_component.h"
#include "game_logic/constants.h"

namespace game_logic::systems {

CollisionSystem::CollisionSystem(engine::event::EventBus &event_bus,
                                 engine::scripting::ScriptEngine &script_engine,
                                 float cell_size)
    : event_bus_(event_bus), script_engine_(script_engine), grid_(cell_size) {}

bool CollisionSystem::CheckOneWayCollision(const engine::math::RectF &a,
                                           const engine::math::RectF &b) {
  return a.Intersects(b);
}

void CollisionSystem::ResolveCollision(engine::ecs::Registry &registry,
                                       engine::ecs::EntityId e1,
                                       engine::ecs::EntityId e2) {
  script_engine_.OnCollision(e1, e2);
  event_bus_.Publish(EntityCollisionEvent{e1, e2});
}

bool CollisionSystem::CheckCompoundCircleVsAABB(
    const engine::ecs::CompoundCircleColliderComponent &compound,
    const engine::math::Vector2f &compound_pos, const engine::math::RectF &box,
    const engine::math::Vector2f &box_pos) {
  engine::math::RectF world_box = box;
  world_box.top_left_x_ += box_pos.x;
  world_box.top_left_y_ += box_pos.y;

  for (const auto &circle : compound.circles) {
    engine::math::Vector2f center = compound_pos + circle.offset;
    if (engine::math::Collision::AABBVsCircle(world_box, center,
                                              circle.radius)) {
      return true;
    }
  }
  return false;
}

bool CollisionSystem::CheckCompoundCircleVsCompoundCircle(
    const engine::ecs::CompoundCircleColliderComponent &compound_a,
    const engine::math::Vector2f &pos_a,
    const engine::ecs::CompoundCircleColliderComponent &compound_b,
    const engine::math::Vector2f &pos_b) {
  for (const auto &circle_a : compound_a.circles) {
    engine::math::Vector2f center_a = pos_a + circle_a.offset;
    for (const auto &circle_b : compound_b.circles) {
      engine::math::Vector2f center_b = pos_b + circle_b.offset;
      if (engine::math::Collision::CircleVsCircle(center_a, circle_a.radius,
                                                  center_b, circle_b.radius)) {
        return true;
      }
    }
  }
  return false;
}

void CollisionSystem::Update(engine::ecs::Registry &registry,
                             engine::time::TimeDelta) {
  grid_.Clear();

  auto &positions = registry.GetComponents<engine::ecs::PositionComponent>();
  auto &boxes = registry.GetComponents<engine::ecs::BoundingBoxComponent>();
  auto &tags = registry.GetComponents<engine::ecs::TagComponent>();
  auto &compounds =
      registry.GetComponents<engine::ecs::CompoundCircleColliderComponent>();

  for (auto &&[entity_idx, pos, tag] :
       engine::ecs::IndexedZipper(positions, tags)) {
    engine::ecs::EntityId entity = registry.EntityFromIndex(entity_idx);

    if (entity_idx < boxes.size() && boxes[entity_idx].has_value()) {
      auto &box = boxes[entity_idx];
      engine::math::RectF world_bounds = box->bounds;
      world_bounds.top_left_x_ += pos->position.x;
      world_bounds.top_left_y_ += pos->position.y;
      grid_.Insert(entity, world_bounds);
    } else if (entity_idx < compounds.size() &&
               compounds[entity_idx].has_value()) {
      auto &compound = compounds[entity_idx];
      float min_x = pos->position.x;
      float min_y = pos->position.y;
      float max_x = pos->position.x;
      float max_y = pos->position.y;

      for (const auto &circle : compound->circles) {
        min_x =
            std::min(min_x, pos->position.x + circle.offset.x - circle.radius);
        min_y =
            std::min(min_y, pos->position.y + circle.offset.y - circle.radius);
        max_x =
            std::max(max_x, pos->position.x + circle.offset.x + circle.radius);
        max_y =
            std::max(max_y, pos->position.y + circle.offset.y + circle.radius);
      }

      engine::math::RectF bounds(min_x, min_y, max_x - min_x, max_y - min_y);
      grid_.Insert(entity, bounds);
    }
  }

  grid_.ForEachPotentialCollision(
      [&](engine::ecs::EntityId e1, engine::ecs::EntityId e2) {
        if (e1 >= positions.size() || e2 >= positions.size()) {
          return;
        }

        auto &pos1 = positions[e1];
        auto &pos2 = positions[e2];

        if (!pos1.has_value() || !pos2.has_value()) {
          return;
        }

        bool has_box1 = e1 < boxes.size() && boxes[e1].has_value();
        bool has_box2 = e2 < boxes.size() && boxes[e2].has_value();
        bool has_compound1 = e1 < compounds.size() && compounds[e1].has_value();
        bool has_compound2 = e2 < compounds.size() && compounds[e2].has_value();

        bool collision = false;

        if (has_compound1 && has_box2) {
          engine::math::RectF rect2 = boxes[e2]->bounds;
          collision = CheckCompoundCircleVsAABB(*compounds[e1], pos1->position,
                                                rect2, pos2->position);
        } else if (has_box1 && has_compound2) {
          engine::math::RectF rect1 = boxes[e1]->bounds;
          collision = CheckCompoundCircleVsAABB(*compounds[e2], pos2->position,
                                                rect1, pos1->position);
        } else if (has_compound1 && has_compound2) {
          collision = CheckCompoundCircleVsCompoundCircle(
              *compounds[e1], pos1->position, *compounds[e2], pos2->position);
        } else if (has_box1 && has_box2) {
          engine::math::RectF rect1 = boxes[e1]->bounds;
          rect1.top_left_x_ += pos1->position.x;
          rect1.top_left_y_ += pos1->position.y;

          engine::math::RectF rect2 = boxes[e2]->bounds;
          rect2.top_left_x_ += pos2->position.x;
          rect2.top_left_y_ += pos2->position.y;

          collision = CheckOneWayCollision(rect1, rect2);
        }

        if (collision) {
          ResolveCollision(registry, e1, e2);
        }
      });
}

}  // namespace game_logic::systems
