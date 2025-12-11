#include "game_logic/entities/player_builder.h"

#include "engine/ecs/component.h"
#include "game_logic/components.h"
#include "game_logic/game_config.h"

namespace game_logic::entities {

engine::ecs::EntityId PlayerBuilder::Create(engine::ecs::Registry& registry,
                                            const PlayerSpawnContext& ctx) {
  const auto& config = GameConfig::Get().GetPlayer();

  engine::ecs::EntityId player = registry.SpawnEntity();

  registry.EmplaceComponent<engine::ecs::PositionComponent>(player,
                                                            ctx.spawn_position);
  registry.EmplaceComponent<engine::ecs::VelocityComponent>(player, 0.0f, 0.0f);

  components::PlayerComponent player_comp;
  player_comp.player_id = ctx.player_id;
  player_comp.room_id = ctx.room_id;
  player_comp.player_slot = ctx.player_slot;
  player_comp.score = 0;
  player_comp.lives = ctx.initial_lives > 0 ? ctx.initial_lives : config.lives;
  registry.AddComponent<components::PlayerComponent>(player,
                                                     std::move(player_comp));

  std::uint32_t health =
      ctx.initial_health > 0 ? ctx.initial_health : config.health;
  registry.EmplaceComponent<components::HealthComponent>(player, health);

  components::WeaponComponent weapon;
  weapon.type = components::WeaponType::kBasic;
  weapon.fire_rate = 2.0f;
  weapon.set_unlimited_ammo();
  game_logic::MissileConfig m_data;
  try {
    m_data = GameConfig::Get().GetMissile("PlayerMissile");
    weapon.projectile_data.name = m_data.name;
    weapon.projectile_data.damage = m_data.damage;
    weapon.projectile_data.fire_rate = weapon.fire_rate;
    weapon.projectile_data.lifetime_seconds = m_data.lifetime_seconds;
    weapon.projectile_data.sprite_width = m_data.sprite_width;
    weapon.projectile_data.sprite_height = m_data.sprite_height;
    weapon.projectile_data.hitbox_scale = m_data.hitbox_scale;
    weapon.projectile_data.texture_path = m_data.texture_path;
    weapon.projectile_data.tint_color = m_data.tint_color;
    weapon.projectile_data.speed = m_data.speed;
    weapon.faction = ProjectileFaction::kPlayer;
  } catch (...) {
  }

  registry.AddComponent<components::WeaponComponent>(player, std::move(weapon));

  const float hitbox_offset_x =
      (config.sprite_width - config.hitbox_width) / 2.0f;
  const float hitbox_offset_y =
      (config.sprite_height - config.hitbox_height) / 2.0f;

  registry.EmplaceComponent<engine::ecs::BoundingBoxComponent>(
      player, hitbox_offset_x, hitbox_offset_y, config.hitbox_width,
      config.hitbox_height);

  components::SpriteComponent sprite;
  sprite.texture_path = config.texture_path;
  sprite.source_rect = engine::math::RectF(0.0f, 0.0f, config.sprite_width,
                                           config.sprite_height);
  sprite.layer = 10;
  sprite.visible = true;
  registry.AddComponent<components::SpriteComponent>(player, std::move(sprite));
  registry.EmplaceComponent<engine::ecs::TagComponent>(player, "Player");

  return player;
}

engine::ecs::EntityId PlayerBuilder::Create(
    engine::ecs::Registry& registry, std::uint32_t player_id,
    std::uint32_t room_id, std::uint8_t player_slot,
    const engine::math::Vector2f& spawn_position) {
  PlayerSpawnContext ctx;
  ctx.player_id = player_id;
  ctx.room_id = room_id;
  ctx.player_slot = player_slot;
  ctx.spawn_position = spawn_position;

  return Create(registry, ctx);
}

}  // namespace game_logic::entities