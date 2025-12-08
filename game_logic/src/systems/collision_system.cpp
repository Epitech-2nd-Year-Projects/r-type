#include "game_logic/systems/collision_system.h"

#include <algorithm>
#include <cmath>
#include <iostream>

#include "engine/ecs/components/bounding_box_component.h"
#include "engine/ecs/components/position_component.h"
#include "engine/ecs/components/tag_component.h"
#include "engine/ecs/indexed_zipper.h"
#include "engine/ecs/registry.h"
#include "game_logic/components/damageable_component.h"
#include "game_logic/components/health_component.h"

namespace game_logic::systems {

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

void CollisionSystem::Update(engine::ecs::Registry& registry,
                             engine::time::TimeDelta) {
  grid_.clear();

  auto& positions = registry.GetComponents<engine::ecs::PositionComponent>();
  auto& boxes = registry.GetComponents<engine::ecs::BoundingBoxComponent>();
  auto& tags = registry.GetComponents<engine::ecs::TagComponent>();
  auto& healths =
      registry.GetComponents<game_logic::components::HealthComponent>();
  auto& damageables =
      registry.GetComponents<game_logic::components::DamageableComponent>();

  for (auto&& [entity_idx, pos, box, tag] :
       engine::ecs::IndexedZipper(positions, boxes, tags)) {
    engine::ecs::EntityId entity = registry.EntityFromIndex(entity_idx);

    engine::math::RectF world_bounds = box->bounds;
    world_bounds.top_left_x_ += pos->position.x;
    world_bounds.top_left_y_ += pos->position.y;

    InsertIntoGrid(entity, world_bounds);
  }

  std::vector<std::pair<engine::ecs::EntityId, engine::ecs::EntityId>>
      checked_pairs;

  auto is_checked = [&](engine::ecs::EntityId a, engine::ecs::EntityId b) {
    if (a > b) std::swap(a, b);
    for (const auto& pair : checked_pairs) {
      if (pair.first == a && pair.second == b) return true;
    }
    return false;
  };

  auto mark_checked = [&](engine::ecs::EntityId a, engine::ecs::EntityId b) {
    if (a > b) std::swap(a, b);
    checked_pairs.emplace_back(a, b);
  };

  for (auto& [key, entities] : grid_) {
    if (entities.size() < 2) continue;

    for (size_t i = 0; i < entities.size(); ++i) {
      for (size_t j = i + 1; j < entities.size(); ++j) {
        engine::ecs::EntityId e1 = entities[i];
        engine::ecs::EntityId e2 = entities[j];

        if (is_checked(e1, e2)) continue;
        mark_checked(e1, e2);

        try {
          auto& tag1 = tags[static_cast<size_t>(e1)];
          auto& tag2 = tags[static_cast<size_t>(e2)];
          auto& pos1 = positions[static_cast<size_t>(e1)];
          auto& pos2 = positions[static_cast<size_t>(e2)];
          auto& box1 = boxes[static_cast<size_t>(e1)];
          auto& box2 = boxes[static_cast<size_t>(e2)];

          if (!tag1 || !tag2 || !pos1 || !pos2 || !box1 || !box2) continue;

          engine::math::RectF rect1 = box1->bounds;
          rect1.top_left_x_ += pos1->position.x;
          rect1.top_left_y_ += pos1->position.y;

          engine::math::RectF rect2 = box2->bounds;
          rect2.top_left_x_ += pos2->position.x;
          rect2.top_left_y_ += pos2->position.y;

          if (CheckOneWayCollision(rect1, rect2)) {
            const std::string& t1 = tag1->tag;
            const std::string& t2 = tag2->tag;

            bool e1_is_player = (t1 == "Player");
            bool e2_is_player = (t2 == "Player");
            bool e1_is_enemy = (t1 == "Enemy");
            bool e2_is_enemy = (t2 == "Enemy");

            auto get_damageable = [&](engine::ecs::EntityId e)
                -> game_logic::components::DamageableComponent* {
              if (static_cast<size_t>(e) < damageables.size()) {
                auto& opt = damageables[static_cast<size_t>(e)];
                if (opt.has_value()) return &opt.value();
              }
              return nullptr;
            };

            auto* dmg1 = get_damageable(e1);
            auto* dmg2 = get_damageable(e2);

            bool e1_is_projectile = (dmg1 != nullptr);
            bool e2_is_projectile = (dmg2 != nullptr);

            auto apply_damage = [&](engine::ecs::EntityId victim,
                                    uint32_t dmg_amount) {
              if (static_cast<size_t>(victim) < healths.size()) {
                auto& hp = healths[static_cast<size_t>(victim)];
                if (hp) {
                  hp->take_damage(dmg_amount);
                  if (!hp->is_alive()) {
                    registry.KillEntity(victim);
                  }
                }
              }
            };
            if ((e1_is_player && e2_is_enemy) ||
                (e1_is_enemy && e2_is_player)) {
              apply_damage(e1, 100);
              apply_damage(e2, 100);
            }
            auto handle_projectile =
                [&](engine::ecs::EntityId proj, engine::ecs::EntityId target,
                    const components::DamageableComponent& dmg_comp) {
                  bool hit = false;
                  if (dmg_comp.faction == 0 &&
                      (tags[static_cast<size_t>(target)]->tag == "Enemy"))
                    hit = true;
                  if (dmg_comp.faction == 1 &&
                      (tags[static_cast<size_t>(target)]->tag == "Player"))
                    hit = true;

                  if (hit) {
                    apply_damage(target, dmg_comp.damage);
                    registry.KillEntity(proj);
                  }
                };

            if (e1_is_projectile && !e2_is_projectile) {
              handle_projectile(e1, e2, *dmg1);
            } else if (!e1_is_projectile && e2_is_projectile) {
              handle_projectile(e2, e1, *dmg2);
            }
          }
        } catch (...) {
          continue;
        }
      }
    }
  }
}

}  // namespace game_logic::systems
