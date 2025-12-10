#include "game_logic/systems/weapon_system.h"

#include "engine/ecs/indexed_zipper.h"
#include "engine/math/vector2.h"
#include "game_logic/entities/missile_builder.h"
#include "game_logic/entities/missile_data.h"

namespace game_logic::systems {

void WeaponSystem::Update(
    engine::ecs::Registry &registry,
    engine::ecs::SparseArray<engine::ecs::PositionComponent> &positions,
    engine::ecs::SparseArray<components::WeaponComponent> &weapons,
    engine::time::TimeDelta dt) {
  for (auto &&[idx, position_opt, weapon_opt] :
       engine::ecs::IndexedZipper(positions, weapons)) {
    if (!position_opt.has_value() || !weapon_opt.has_value()) {
      continue;
    }

    auto &position = position_opt.value();
    auto &weapon = weapon_opt.value();

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
      spawn_position.x -= 16.0f;
      speed_multiplier = -1.0f;
    } else {
      spawn_position.x += 16.0f;
    }

    if (weapon.is_big_trigger_held && weapon.can_fire_big()) {
      weapon.fire_big(entities::kBigPlayerMissileData.fire_rate);
      engine::math::Vector2f missile_velocity(250.0f * speed_multiplier, 0.0f);
      entities::MissileBuilder::CreateMissile(
          registry, static_cast<std::uint32_t>(idx), spawn_position,
          missile_velocity, entities::kBigPlayerMissileData, weapon.faction);
    }

    if (weapon.is_trigger_held && weapon.can_fire()) {
      weapon.fire(weapon.projectile_data.fire_rate);
      engine::math::Vector2f missile_velocity(300.0f * speed_multiplier, 0.0f);
      entities::MissileBuilder::CreateMissile(
          registry, static_cast<std::uint32_t>(idx), spawn_position,
          missile_velocity, weapon.projectile_data, weapon.faction);
    }
  }
}

}  // namespace game_logic::systems
