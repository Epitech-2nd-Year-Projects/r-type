#include "game_logic/systems/weapon_system.h"

#include "engine/ecs/components/position_component.h"
#include "engine/ecs/components/velocity_component.h"
#include "engine/ecs/indexed_zipper.h"
#include "engine/math/vector2.h"
#include "game_logic/components.h"

namespace game_logic::systems {

void WeaponSystem::Update(
    engine::ecs::Registry &registry,
    engine::ecs::SparseArray<engine::ecs::PositionComponent> &positions,
    engine::ecs::SparseArray<components::WeaponComponent> &weapons,
    engine::ecs::SparseArray<components::SpriteComponent> &sprites,
    engine::time::TimeDelta dt,
    engine::scripting::PrefabFactory &prefab_factory) {
  for (auto &&[idx, position_opt, weapon_opt, sprite_opt] :
       engine::ecs::IndexedZipper(positions, weapons, sprites)) {
    if (!position_opt.has_value() || !weapon_opt.has_value()) {
      continue;
    }

    auto &position = position_opt.value();
    auto &weapon = weapon_opt.value();

    float spawn_offset_x = 16.0f;
    if (sprite_opt.has_value()) {
      spawn_offset_x = sprite_opt->source_rect.width_ / 2.0f;
    }

    if (weapon.cooldown_remaining > engine::time::TimeDelta::zero()) {
      weapon.cooldown_remaining -= dt;
      if (weapon.cooldown_remaining < engine::time::TimeDelta::zero()) {
        weapon.cooldown_remaining = engine::time::TimeDelta::zero();
      }
    }

    if (weapon.big_shot_cooldown_remaining > engine::time::TimeDelta::zero()) {
      weapon.big_shot_cooldown_remaining -= dt;
      if (weapon.big_shot_cooldown_remaining <
          engine::time::TimeDelta::zero()) {
        weapon.big_shot_cooldown_remaining = engine::time::TimeDelta::zero();
      }
    }

    engine::math::Vector2f spawn_position = position.position;

    float speed_multiplier = 1.0f;
    if (weapon.faction == entities::ProjectileFaction::kEnemy) {
      spawn_position.x -= spawn_offset_x;
      speed_multiplier = -1.0f;
    } else {
      spawn_position.x += spawn_offset_x;
    }

    if (weapon.is_big_trigger_held && weapon.can_fire_big()) {
      if (!weapon.big_projectile_prefab.empty()) {
        weapon.fire_big(1.0f);
        auto opt_missile =
            prefab_factory.Spawn(registry, weapon.big_projectile_prefab);
        if (opt_missile) {
          engine::ecs::EntityId missile = *opt_missile;
          registry.EmplaceComponent<engine::ecs::PositionComponent>(
              missile, spawn_position.x, spawn_position.y);
          if (weapon.big_projectile_speed > 0.0f) {
            registry.EmplaceComponent<engine::ecs::VelocityComponent>(
                missile, weapon.big_projectile_speed * speed_multiplier, 0.0f);
          }
          try {
            auto &damageables =
                registry.GetComponents<components::DamageableComponent>();
            if (static_cast<size_t>(missile) < damageables.size() &&
                damageables[missile].has_value()) {
              damageables[missile]->owner_id = static_cast<std::uint32_t>(idx);
            }
          } catch (...) {
          }
        }
      }
    }

    if (weapon.is_trigger_held && weapon.can_fire()) {
      if (!weapon.projectile_prefab.empty()) {
        weapon.fire(weapon.fire_rate);
        auto opt_missile =
            prefab_factory.Spawn(registry, weapon.projectile_prefab);
        if (opt_missile) {
          engine::ecs::EntityId missile = *opt_missile;
          registry.EmplaceComponent<engine::ecs::PositionComponent>(
              missile, spawn_position.x, spawn_position.y);

          float speed = weapon.projectile_speed;
          if (speed <= 0.0f) speed = 300.0f;

          registry.EmplaceComponent<engine::ecs::VelocityComponent>(
              missile, speed * speed_multiplier, 0.0f);

          try {
            auto &damageables =
                registry.GetComponents<components::DamageableComponent>();
            if (static_cast<size_t>(missile) < damageables.size() &&
                damageables[missile].has_value()) {
              damageables[missile]->owner_id = static_cast<std::uint32_t>(idx);
            }
          } catch (...) {
          }
        }
      }
    }
  }
}

}  // namespace game_logic::systems
