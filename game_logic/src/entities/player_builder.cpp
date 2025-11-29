#include "game_logic/entities/player_builder.h"

#include "engine/ecs/component.h"
#include "game_logic/components.h"

namespace game_logic::entities {

engine::ecs::EntityId PlayerBuilder::Create(engine::ecs::Registry& registry,
                                            const PlayerConfig& config) {
  engine::ecs::EntityId player = registry.SpawnEntity();

  registry.EmplaceComponent<engine::ecs::PositionComponent>(
      player, config.spawn_position);
  registry.EmplaceComponent<engine::ecs::VelocityComponent>(player, 0.0f, 0.0f);
  components::PlayerComponent player_comp;
  player_comp.player_id = config.player_id;
  player_comp.room_id = config.room_id;
  player_comp.player_slot = config.player_slot;
  player_comp.score = 0;
  player_comp.lives = config.initial_lives;
  registry.AddComponent<components::PlayerComponent>(player,
                                                     std::move(player_comp));
  registry.EmplaceComponent<components::HealthComponent>(player,
                                                         config.initial_health);
  components::WeaponComponent weapon;
  weapon.type = components::WeaponType::kBasic;
  weapon.fire_rate = 2.0f;
  weapon.set_unlimited_ammo();
  weapon.power_level = 1;
  registry.AddComponent<components::WeaponComponent>(player, std::move(weapon));

  const float hitbox_offset_x = (kPlayerWidth - kHitboxWidth) / 2.0f;
  const float hitbox_offset_y = (kPlayerHeight - kHitboxHeight) / 2.0f;

  registry.EmplaceComponent<engine::ecs::BoundingBoxComponent>(
      player, hitbox_offset_x, hitbox_offset_y, kHitboxWidth, kHitboxHeight);

  components::SpriteComponent sprite;
  sprite.texture_path = kPlayerTexturePath;
  sprite.source_rect =
      engine::math::RectF(0.0f, 0.0f, kPlayerWidth, kPlayerHeight);
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
  PlayerConfig config;
  config.player_id = player_id;
  config.room_id = room_id;
  config.player_slot = player_slot;
  config.spawn_position = spawn_position;

  return Create(registry, config);
}

}  // namespace game_logic::entities