#include "game_logic/entities/enemy_builder.h"

#include "engine/ecs/component.h"
#include "game_logic/components.h"
#include "game_logic/entities/enemy_data.h"
#include "game_logic/entities/missile_config.h"
#include "game_logic/entities/missile_data.h"

namespace game_logic::entities {

const EnemyArchetypeData &GetArchetypeData(EnemyType type) {
  switch (type) {
    case EnemyType::kScout:
      return kScoutData;
    case EnemyType::kBomber:
      return kBomberData;
    case EnemyType::kTank:
      return kTankData;
    case EnemyType::kInterceptor:
      return kInterceptorData;
    default:
      return kScoutData;
  }
}

engine::ecs::EntityId EnemyBuilder::Create(engine::ecs::Registry &registry,
                                           const EnemyConfig &config) {
  const auto &data = GetArchetypeData(config.type);

  engine::ecs::EntityId enemy = registry.SpawnEntity();

  registry.EmplaceComponent<engine::ecs::PositionComponent>(
      enemy, config.spawn_position);

  engine::math::Vector2f velocity = config.initial_velocity;
  if (velocity.x == 0.0f && velocity.y == 0.0f) {
    velocity.x = -data.speed;
    velocity.y = 0.0f;
  }
  registry.EmplaceComponent<engine::ecs::VelocityComponent>(enemy, velocity);

  std::uint32_t health =
      config.custom_health > 0 ? config.custom_health : data.health;
  registry.EmplaceComponent<components::HealthComponent>(enemy, health);

  components::AIComponent ai;
  ai.behavior =
      config.use_custom_behavior ? config.custom_behavior : data.behavior;

  ai.speed = config.custom_speed > 0.0f ? config.custom_speed : data.speed;
  ai.detection_range = data.detection_range;
  ai.wave_amplitude = data.wave_amplitude;
  ai.wave_frequency = data.wave_frequency;

  if (ai.behavior == components::EnemyBehavior::kPatrol) {
    ai.patrol_min = engine::math::Vector2f{0.0f, 100.0f};
    ai.patrol_max = engine::math::Vector2f{800.0f, 500.0f};
  }

  registry.AddComponent<components::AIComponent>(enemy, std::move(ai));

  std::uint32_t score =
      config.custom_score > 0 ? config.custom_score : data.score;
  registry.EmplaceComponent<components::ScoreValueComponent>(enemy, score);

  const float hitbox_offset_x = (data.sprite_width - data.hitbox_width) / 2.0f;
  const float hitbox_offset_y =
      (data.sprite_height - data.hitbox_height) / 2.0f;

  registry.EmplaceComponent<engine::ecs::BoundingBoxComponent>(
      enemy, hitbox_offset_x, hitbox_offset_y, data.hitbox_width,
      data.hitbox_height);

  components::SpriteComponent sprite;
  sprite.texture_path = data.texture_path;
  sprite.source_rect =
      engine::math::RectF(0.0f, 0.0f, data.sprite_width, data.sprite_height);
  sprite.layer = 5;
  sprite.visible = true;
  sprite.tint.r = 255;
  sprite.tint.g = 80;
  sprite.tint.b = 80;
  sprite.tint.a = 255;

  registry.AddComponent<components::SpriteComponent>(enemy, std::move(sprite));

  if (data.can_shoot) {
    components::WeaponComponent weapon;
    weapon.projectile_data = kEnemyMissileData;
    weapon.projectile_data.fire_rate = data.fire_rate;
    weapon.faction = ProjectileFaction::kEnemy;
    weapon.set_unlimited_ammo();
    weapon.is_trigger_held = true;
    weapon.cooldown_remaining = engine::time::TimeDelta::from_seconds(0.0f);

    registry.AddComponent<components::WeaponComponent>(enemy,
                                                       std::move(weapon));
  }

  registry.EmplaceComponent<engine::ecs::TagComponent>(enemy, "Enemy");

  return enemy;
}

engine::ecs::EntityId EnemyBuilder::Create(
    engine::ecs::Registry &registry, EnemyType type,
    const engine::math::Vector2f &spawn_position) {
  EnemyConfig config;
  config.type = type;
  config.spawn_position = spawn_position;
  return Create(registry, config);
}

engine::ecs::EntityId EnemyBuilder::CreatePataPata(
    engine::ecs::Registry &registry,
    const engine::math::Vector2f &spawn_position) {
  return Create(registry, EnemyType::kScout, spawn_position);
}

engine::ecs::EntityId EnemyBuilder::CreateBydo(
    engine::ecs::Registry &registry,
    const engine::math::Vector2f &spawn_position) {
  return Create(registry, EnemyType::kBomber, spawn_position);
}

}  // namespace game_logic::entities