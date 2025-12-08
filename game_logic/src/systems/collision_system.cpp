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

        if (static_cast<size_t>(e1) >= tags.size() ||
            static_cast<size_t>(e2) >= tags.size() ||
            static_cast<size_t>(e1) >= positions.size() ||
            static_cast<size_t>(e2) >= positions.size() ||
            static_cast<size_t>(e1) >= boxes.size() ||
            static_cast<size_t>(e2) >= boxes.size()) {
          continue;
        }

        auto& tag1 = tags[static_cast<size_t>(e1)];
        auto& tag2 = tags[static_cast<size_t>(e2)];
        auto& pos1 = positions[static_cast<size_t>(e1)];
        auto& pos2 = positions[static_cast<size_t>(e2)];
        auto& box1 = boxes[static_cast<size_t>(e1)];
        auto& box2 = boxes[static_cast<size_t>(e2)];

        if (!tag1.has_value() || !tag2.has_value() || !pos1.has_value() ||
            !pos2.has_value() || !box1.has_value() || !box2.has_value())
          continue;

        engine::math::RectF rect1 = box1->bounds;
        rect1.top_left_x_ += pos1->position.x;
        rect1.top_left_y_ += pos1->position.y;

        engine::math::RectF rect2 = box2->bounds;
        rect2.top_left_x_ += pos2->position.x;
        rect2.top_left_y_ += pos2->position.y;

        if (CheckOneWayCollision(rect1, rect2)) {
          const std::string& t1 = tag1->tag;
          const std::string& t2 = tag2->tag;

          bool e1_is_player = (t1 == kPlayerTag);
          bool e2_is_player = (t2 == kPlayerTag);
          bool e1_is_enemy = (t1 == kEnemyTag);
          bool e2_is_enemy = (t2 == kEnemyTag);

          auto get_damageable = [&](engine::ecs::EntityId e)
              -> std::optional<game_logic::components::DamageableComponent> {
            if (static_cast<size_t>(e) < damageables.size()) {
              return damageables[static_cast<size_t>(e)];
            }
            return std::nullopt;
          };

          auto dmg1 = get_damageable(e1);
          auto dmg2 = get_damageable(e2);

          bool e1_has_damage = dmg1.has_value();
          bool e2_has_damage = dmg2.has_value();

          auto apply_damage = [&](engine::ecs::EntityId victim,
                                  uint32_t dmg_amount) {
            if (static_cast<size_t>(victim) < healths.size()) {
              auto& hp = healths[static_cast<size_t>(victim)];
              if (hp.has_value()) {
                hp->take_damage(dmg_amount);
                if (!hp->is_alive()) {
                  registry.KillEntity(victim);
                }
              }
            }
          };

          if ((e1_is_player && e2_is_enemy) || (e1_is_enemy && e2_is_player)) {
            apply_damage(e1, kCrashDamage);
            apply_damage(e2, kCrashDamage);
          }

          auto handle_projectile =
              [&](engine::ecs::EntityId proj, engine::ecs::EntityId target,
                  const components::DamageableComponent& dmg_comp) {
                if (static_cast<size_t>(target) >= tags.size()) return;
                auto& target_tag_opt = tags[static_cast<size_t>(target)];
                if (!target_tag_opt.has_value()) return;

                bool hit = false;
                if (dmg_comp.faction == 0 &&
                    (target_tag_opt->tag == kEnemyTag)) {
                  hit = true;
                }
                if (dmg_comp.faction == 1 &&
                    (target_tag_opt->tag == kPlayerTag)) {
                  hit = true;
                }

                if (hit) {
                  apply_damage(target, dmg_comp.damage);
                  registry.KillEntity(proj);
                }
              };

          if (e1_has_damage && !e2_has_damage) {
            handle_projectile(e1, e2, dmg1.value());
          } else if (!e1_has_damage && e2_has_damage) {
            handle_projectile(e2, e1, dmg2.value());
          }
        }
      }
    }
  }
}

}  // namespace game_logic::systems
