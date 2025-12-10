#include "game_logic/systems/weapon_system.h"

#include "engine/ecs/indexed_zipper.h"
#include "engine/math/vector2.h"
#include "game_logic/entities/missile_builder.h"

namespace game_logic::systems {

void WeaponSystem::Update(
    engine::ecs::Registry &registry,
    engine::ecs::SparseArray<engine::ecs::PositionComponent> &positions,
    engine::ecs::SparseArray<components::PlayerComponent> &players,
    engine::ecs::SparseArray<components::WeaponComponent> &weapons,
    engine::time::TimeDelta dt) {
  for (auto &&[idx, position_opt, player_opt, weapon_opt] :
       engine::ecs::IndexedZipper(positions, players, weapons)) {
    if (!position_opt.has_value() || !player_opt.has_value() ||
        !weapon_opt.has_value()) {
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

    if (!weapon.is_trigger_held) {
      continue;
    }

    if (!weapon.can_fire()) {
      continue;
    }

    weapon.fire();

    engine::math::Vector2f spawn_position = position.position;
    spawn_position.x += 16.0f;

    engine::math::Vector2f missile_velocity(300.0f, 0.0f);

    entities::MissileBuilder::CreatePlayerMissile(
        registry, static_cast<std::uint32_t>(idx), spawn_position,
        missile_velocity);
  }
}

}  // namespace game_logic::systems
