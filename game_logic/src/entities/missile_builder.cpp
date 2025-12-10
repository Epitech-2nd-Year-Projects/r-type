#include "game_logic/entities/missile_builder.h"

#include "engine/ecs/component.h"
#include "game_logic/components.h"
#include "game_logic/entities/missile_data.h"

namespace game_logic::entities {

engine::ecs::EntityId MissileBuilder::Create(
    engine::ecs::Registry &registry, const MissileConfig &config,
    const MissileArchetypeData &archetype) {
  engine::ecs::EntityId missile = registry.SpawnEntity();

  registry.EmplaceComponent<engine::ecs::PositionComponent>(
      missile, config.spawn_position);

  registry.EmplaceComponent<engine::ecs::VelocityComponent>(missile,
                                                            config.velocity);

  components::DamageableComponent damageable;
  damageable.owner_id = config.owner_id;
  damageable.damage = config.damage;
  damageable.faction = static_cast<std::uint8_t>(config.faction);
  damageable.friendly_fire = config.friendly_fire;
  registry.AddComponent<components::DamageableComponent>(missile,
                                                         std::move(damageable));

  registry.EmplaceComponent<engine::ecs::LifetimeComponent>(
      missile, engine::time::TimeDelta::from_seconds(config.lifetime));

  const float hitbox_width = config.sprite_width * archetype.hitbox_scale;
  const float hitbox_height = config.sprite_height * archetype.hitbox_scale;
  const float hitbox_offset_x = (config.sprite_width - hitbox_width) / 2.0f;
  const float hitbox_offset_y = (config.sprite_height - hitbox_height) / 2.0f;

  registry.EmplaceComponent<engine::ecs::BoundingBoxComponent>(
      missile, hitbox_offset_x, hitbox_offset_y, hitbox_width, hitbox_height);

  components::SpriteComponent sprite;
  sprite.texture_path = archetype.texture_path;
  sprite.source_rect = engine::math::RectF(0.0f, 0.0f, config.sprite_width,
                                           config.sprite_height);
  sprite.layer = 8;
  sprite.visible = true;
  sprite.tint.r = static_cast<std::uint8_t>(archetype.tint_color.r * 255.0f);
  sprite.tint.g = static_cast<std::uint8_t>(archetype.tint_color.g * 255.0f);
  sprite.tint.b = static_cast<std::uint8_t>(archetype.tint_color.b * 255.0f);
  sprite.tint.a = static_cast<std::uint8_t>(archetype.tint_color.a * 255.0f);

  registry.AddComponent<components::SpriteComponent>(missile,
                                                     std::move(sprite));

  registry.EmplaceComponent<engine::ecs::TagComponent>(missile, "Missile");

  return missile;
}

engine::ecs::EntityId MissileBuilder::CreateMissile(
    engine::ecs::Registry &registry, std::uint32_t owner_id,
    const engine::math::Vector2f &spawn_position,
    const engine::math::Vector2f &velocity,
    const MissileArchetypeData &archetype, ProjectileFaction faction) {
  MissileConfig config;
  config.spawn_position = spawn_position;
  config.velocity = velocity;
  config.damage = archetype.damage;
  config.lifetime = archetype.lifetime_seconds;
  config.owner_id = owner_id;
  config.faction = faction;
  config.friendly_fire = false;
  config.sprite_width = archetype.sprite_width;
  config.sprite_height = archetype.sprite_height;

  return Create(registry, config, archetype);
}

}  // namespace game_logic::entities