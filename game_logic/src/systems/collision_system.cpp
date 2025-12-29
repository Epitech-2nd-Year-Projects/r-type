#include "game_logic/systems/collision_system.h"

#include <algorithm>
#include <cmath>
#include <unordered_set>

#include "engine/ecs/components/bounding_box_component.h"
#include "engine/ecs/components/position_component.h"
#include "engine/ecs/components/tag_component.h"
#include "engine/ecs/indexed_zipper.h"
#include "engine/ecs/registry.h"
#include "engine/event.h"
#include "engine/scripting/script_engine.h"
#include "game_logic/components/damageable_component.h"
#include "game_logic/components/health_component.h"
#include "game_logic/constants.h"

namespace game_logic::systems {

CollisionSystem::CollisionSystem(engine::event::EventBus& event_bus,
                                 engine::scripting::ScriptEngine& script_engine,
                                 float cell_size)
    : event_bus_(event_bus),
      script_engine_(script_engine),
      grid_(cell_size) {}

bool CollisionSystem::CheckOneWayCollision(const engine::math::RectF& a,
                                           const engine::math::RectF& b) {
  return a.Intersects(b);
}

void CollisionSystem::ResolveCollision(engine::ecs::Registry&,
                                       engine::ecs::EntityId e1,
                                       engine::ecs::EntityId e2) {
  sol::state& lua = script_engine_.LuaState();
  sol::table game_events = lua["GameEvents"];
  if (game_events.valid()) {
    sol::function handle_collision = game_events["HandleCollision"];
    if (handle_collision.valid()) {
      handle_collision(e1, e2);
    }
  }

  // Still publish event for other systems (Audio etc.)
  event_bus_.Publish(EntityCollisionEvent{e1, e2});
}

// Deprecated / Unused logic removed
void CollisionSystem::ResolveProjectile(
    engine::ecs::Registry&, engine::ecs::EntityId, engine::ecs::EntityId,
    const game_logic::components::DamageableComponent&) {}

void CollisionSystem::Update(engine::ecs::Registry& registry,
                             engine::time::TimeDelta) {
  grid_.Clear();

  auto& positions = registry.GetComponents<engine::ecs::PositionComponent>();
  auto& boxes = registry.GetComponents<engine::ecs::BoundingBoxComponent>();
  auto& tags = registry.GetComponents<engine::ecs::TagComponent>();

  for (auto&& [entity_idx, pos, box, tag] :
       engine::ecs::IndexedZipper(positions, boxes, tags)) {
    engine::ecs::EntityId entity = registry.EntityFromIndex(entity_idx);

    engine::math::RectF world_bounds = box->bounds;
    world_bounds.top_left_x_ += pos->position.x;
    world_bounds.top_left_y_ += pos->position.y;

    grid_.Insert(entity, world_bounds);
  }

  grid_.ForEachPotentialCollision(
      [&](engine::ecs::EntityId e1, engine::ecs::EntityId e2) {
        if (e1 >= positions.size() || e2 >= positions.size() ||
            e1 >= boxes.size() || e2 >= boxes.size()) {
          return;
        }

        auto& pos1 = positions[e1];
        auto& pos2 = positions[e2];
        auto& box1 = boxes[e1];
        auto& box2 = boxes[e2];

        if (!pos1.has_value() || !pos2.has_value() || !box1.has_value() ||
            !box2.has_value())
          return;

        engine::math::RectF rect1 = box1->bounds;
        rect1.top_left_x_ += pos1->position.x;
        rect1.top_left_y_ += pos1->position.y;

        engine::math::RectF rect2 = box2->bounds;
        rect2.top_left_x_ += pos2->position.x;
        rect2.top_left_y_ += pos2->position.y;

        if (CheckOneWayCollision(rect1, rect2)) {
          ResolveCollision(registry, e1, e2);
        }
      });
}

}  // namespace game_logic::systems
