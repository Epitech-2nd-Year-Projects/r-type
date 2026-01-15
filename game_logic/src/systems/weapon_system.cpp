#include "game_logic/systems/weapon_system.h"

#include <sol/sol.hpp>

#include "engine/ecs/components/position_component.h"
#include "engine/ecs/components/velocity_component.h"
#include "engine/ecs/indexed_zipper.h"
#include "engine/scripting/script_engine.h"
#include "game_logic/components.h"
#include "game_logic/components/shoot_event_component.h"

namespace game_logic::systems {

WeaponSystem::WeaponSystem(engine::scripting::ScriptEngine& script_engine)
    : script_engine_(script_engine) {}

void WeaponSystem::Update(engine::ecs::Registry& registry,
                          engine::time::TimeDelta dt) {
  auto& positions = registry.GetComponents<engine::ecs::PositionComponent>();
  auto& weapons =
      registry.GetComponents<game_logic::components::WeaponComponent>();
  auto& sprites =
      registry.GetComponents<game_logic::components::SpriteComponent>();
  auto& shoot_events =
      registry.GetComponents<game_logic::components::ShootEventComponent>();

  engine::scripting::PrefabFactory& prefab_factory =
      script_engine_.GetPrefabFactory();
  sol::state& lua = script_engine_.LuaState();
  sol::table weapon_logic = lua["WeaponLogic"];

  for (auto&& [idx, position_opt, weapon_opt] :
       engine::ecs::IndexedZipper(positions, weapons)) {
    if (!position_opt.has_value() || !weapon_opt.has_value()) {
      continue;
    }

    auto& position = position_opt.value();
    auto& weapon = weapon_opt.value();

    if (weapon.rapid_fire_timer > 0.0f) {
      if (weapon.original_fire_rate <= 0.0f) {
        weapon.original_fire_rate = weapon.fire_rate;
        if (weapon.original_fire_rate <= 0.0f) weapon.original_fire_rate = 2.0f;
        weapon.fire_rate = 15.0f;
      }
      weapon.rapid_fire_timer -= dt.as_seconds();
      if (weapon.rapid_fire_timer <= 0.0f) {
        weapon.fire_rate = weapon.original_fire_rate;
        weapon.original_fire_rate = 0.0f;
        weapon.rapid_fire_timer = 0.0f;
      }
    }

    bool use_lag_comp = false;
    engine::math::Vector2f lc_position = position.position;
    float lc_latency = 0.0f;

    if (idx < shoot_events.size() && shoot_events[idx].has_value()) {
      auto& evt = shoot_events[idx].value();
      if (evt.fired) {
        use_lag_comp = true;
        lc_position = evt.spawn_position;
        lc_latency = evt.latency_s;
        evt.fired = false;
      }
    }

    if (!weapon.weapon_script.empty() && weapon_logic.valid()) {
      sol::function script_func = weapon_logic[weapon.weapon_script];
      if (script_func.valid()) {
        engine::ecs::EntityId entity = registry.EntityFromIndex(idx);
        script_func(entity, dt.as_seconds(), &weapon, &position);
        continue;
      }
    }

    float spawn_offset_x = 16.0f;
    if (idx < sprites.size() && sprites[idx].has_value()) {
      spawn_offset_x = sprites[idx]->source_rect.width_ / 2.0f;
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

    engine::math::Vector2f spawn_position = lc_position;

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

          float missile_vx = weapon.big_projectile_speed * speed_multiplier;
          if (weapon.big_projectile_speed > 0.0f) {
            registry.EmplaceComponent<engine::ecs::VelocityComponent>(
                missile, missile_vx, 0.0f);
          }

          if (use_lag_comp && lc_latency > 0.0f) {
            auto& pos_comp =
                registry
                    .GetComponents<engine::ecs::PositionComponent>()[missile]
                    .value();
            pos_comp.position.x += missile_vx * lc_latency;
          }

          try {
            auto& damageables =
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
          float missile_vx = speed * speed_multiplier;

          registry.EmplaceComponent<engine::ecs::VelocityComponent>(
              missile, missile_vx, 0.0f);

          if (use_lag_comp && lc_latency > 0.0f) {
            auto& pos_comp =
                registry
                    .GetComponents<engine::ecs::PositionComponent>()[missile]
                    .value();
            pos_comp.position.x += missile_vx * lc_latency;
          }

          try {
            auto& damageables =
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
