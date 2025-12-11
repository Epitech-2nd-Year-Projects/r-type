#include "game_logic/systems/collision_system.h"

#include <algorithm>
#include <cmath>
#include <unordered_set>

#include "engine/ecs/components/bounding_box_component.h"
#include "engine/ecs/components/position_component.h"
#include "engine/ecs/components/tag_component.h"
#include "engine/ecs/indexed_zipper.h"
#include "engine/ecs/registry.h"
#include "game_logic/components/damageable_component.h"
#include "game_logic/components/health_component.h"
#include "game_logic/constants.h"

namespace game_logic::systems {

namespace {
inline std::uint64_t PairHash(engine::ecs::EntityId a,
                              engine::ecs::EntityId b) {
  if (a > b) std::swap(a, b);
  return (static_cast<std::uint64_t>(a) << 32) | static_cast<std::uint64_t>(b);
}
}  // namespace

CollisionSystem::CollisionSystem(float cell_size) : cell_size_(cell_size) {}

void CollisionSystem::InsertIntoGrid(engine::ecs::EntityId entity,
                                     const engine::math::RectF& bounds) {
  int min_x = static_cast<int>(std::floor(bounds.top_left_x_ / cell_size_));
  int max_x = static_cast<int>(
      std::floor((bounds.top_left_x_ + bounds.width_) / cell_size_));
  int min_y = static_cast<int>(std::floor(bounds.top_left_y_ / cell_size_));
  int max_y = static_cast<int>(
      std::floor((bounds.top_left_y_ + bounds.height_) / cell_size_));

  for (int x = min_x; x <= max_x; ++x) {
    for (int y = min_y; y <= max_y; ++y) {
      grid_[{x, y}].push_back(entity);
    }
  }
}

bool CollisionSystem::CheckOneWayCollision(const engine::math::RectF& a,
                                           const engine::math::RectF& b) {
  return a.Intersects(b);
}

void CollisionSystem::ResolveCollision(engine::ecs::Registry& registry,
                                       engine::ecs::EntityId e1,
                                       engine::ecs::EntityId e2) {
  auto& tags = registry.GetComponents<engine::ecs::TagComponent>();
  auto& damageables =
      registry.GetComponents<game_logic::components::DamageableComponent>();
  auto& healths =
      registry.GetComponents<game_logic::components::HealthComponent>();

  if (e1 >= tags.size() || e2 >= tags.size()) return;

  auto& tag1 = tags[e1];
  auto& tag2 = tags[e2];

  if (!tag1.has_value() || !tag2.has_value()) return;

  const std::string& t1 = tag1->tag;
  const std::string& t2 = tag2->tag;

  bool e1_is_player = (t1 == kPlayerTag);
  bool e2_is_player = (t2 == kPlayerTag);
  bool e1_is_enemy = (t1 == kEnemyTag);
  bool e2_is_enemy = (t2 == kEnemyTag);

  if ((e1_is_player && e2_is_enemy) || (e1_is_enemy && e2_is_player)) {
    auto apply_crash = [&](engine::ecs::EntityId victim,
                           engine::ecs::EntityId attacker) {
      if (victim < healths.size() && healths[victim].has_value()) {
        healths[victim]->take_damage(game_logic::kCrashDamage);
        healths[victim]->last_attacker_id = attacker;
      }
    };
    apply_crash(e1, e2);
    apply_crash(e2, e1);
  }

  bool has_dmg1 = (e1 < damageables.size() && damageables[e1].has_value());
  bool has_dmg2 = (e2 < damageables.size() && damageables[e2].has_value());

  if (has_dmg1 && !has_dmg2) {
    ResolveProjectile(registry, e1, e2, damageables[e1].value());
  } else if (!has_dmg1 && has_dmg2) {
    ResolveProjectile(registry, e2, e1, damageables[e2].value());
  }
}

void CollisionSystem::ResolveProjectile(
    engine::ecs::Registry& registry, engine::ecs::EntityId proj,
    engine::ecs::EntityId target,
    const game_logic::components::DamageableComponent& dmg_comp) {
  auto& tags = registry.GetComponents<engine::ecs::TagComponent>();
  auto& healths =
      registry.GetComponents<game_logic::components::HealthComponent>();

  if (target >= tags.size()) return;
  auto& target_tag = tags[target];
  if (!target_tag.has_value()) return;

  bool hit = false;

  if (dmg_comp.faction == 0 && target_tag->tag == kEnemyTag) {
    hit = true;
  }
  if (dmg_comp.faction == 1 && target_tag->tag == kPlayerTag) {
    hit = true;
  }

  if (hit) {
    if (target < healths.size() && healths[target].has_value()) {
      healths[target]->take_damage(dmg_comp.damage);
      healths[target]->last_attacker_id = dmg_comp.owner_id;
    }
    registry.KillEntity(proj);
  }
}

void CollisionSystem::Update(engine::ecs::Registry& registry,
                             engine::time::TimeDelta) {
  grid_.clear();

  auto& positions = registry.GetComponents<engine::ecs::PositionComponent>();
  auto& boxes = registry.GetComponents<engine::ecs::BoundingBoxComponent>();
  auto& tags = registry.GetComponents<engine::ecs::TagComponent>();

  for (auto&& [entity_idx, pos, box, tag] :
       engine::ecs::IndexedZipper(positions, boxes, tags)) {
    engine::ecs::EntityId entity = registry.EntityFromIndex(entity_idx);

    engine::math::RectF world_bounds = box->bounds;
    world_bounds.top_left_x_ += pos->position.x;
    world_bounds.top_left_y_ += pos->position.y;

    InsertIntoGrid(entity, world_bounds);
  }

  std::unordered_set<std::uint64_t> checked_pairs;

  for (auto& [key, entities] : grid_) {
    if (entities.size() < 2) continue;

    for (size_t i = 0; i < entities.size(); ++i) {
      for (size_t j = i + 1; j < entities.size(); ++j) {
        engine::ecs::EntityId e1 = entities[i];
        engine::ecs::EntityId e2 = entities[j];

        std::uint64_t pair_hash = PairHash(e1, e2);
        if (checked_pairs.count(pair_hash)) continue;
        checked_pairs.insert(pair_hash);

        if (e1 >= positions.size() || e2 >= positions.size() ||
            e1 >= boxes.size() || e2 >= boxes.size()) {
          continue;
        }

        auto& pos1 = positions[e1];
        auto& pos2 = positions[e2];
        auto& box1 = boxes[e1];
        auto& box2 = boxes[e2];

        if (!pos1.has_value() || !pos2.has_value() || !box1.has_value() ||
            !box2.has_value())
          continue;

        engine::math::RectF rect1 = box1->bounds;
        rect1.top_left_x_ += pos1->position.x;
        rect1.top_left_y_ += pos1->position.y;

        engine::math::RectF rect2 = box2->bounds;
        rect2.top_left_x_ += pos2->position.x;
        rect2.top_left_y_ += pos2->position.y;

        if (CheckOneWayCollision(rect1, rect2)) {
          ResolveCollision(registry, e1, e2);
        }
      }
    }
  }
}

}  // namespace game_logic::systems
