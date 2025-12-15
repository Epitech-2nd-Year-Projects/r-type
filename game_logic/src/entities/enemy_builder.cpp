#include "game_logic/entities/enemy_builder.h"

#include "engine/ecs/component.h"
#include "game_logic/components.h"
#include "game_logic/components/powerup_drop_component.h"
#include "game_logic/constants.h"
#include "game_logic/entities/enemy_data.h"
#include "game_logic/entities/missile_config.h"
#include "game_logic/entities/missile_data.h"
#include "game_logic/game_config.h"

namespace game_logic::entities {

std::string GetEnemyName(EnemyType type) {
  switch (type) {
    case EnemyType::kScout:
      return "Scout";
    case EnemyType::kBomber:
      return "Bomber";
    case EnemyType::kTank:
      return "Tank";
    case EnemyType::kInterceptor:
      return "Interceptor";
    default:
      return "Scout";
  }
}

engine::ecs::EntityId EnemyBuilder::Create(engine::ecs::Registry &registry,
                                           const EnemySpawnConfig &config) {
  const EnemyConfig &data = [&]() -> const EnemyConfig & {
    try {
      return GameConfig::Get().GetEnemy(GetEnemyName(config.type));
    } catch (const std::exception &) {
      try {
        return GameConfig::Get().GetEnemy("Scout");
      } catch (const std::exception &) {
        static const EnemyConfig kDefaultEnemy{};
        return kDefaultEnemy;
      }
    }
  }();

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
  if (data.behavior_type == "Patrol")
    ai.behavior = components::EnemyBehavior::kPatrol;
  else if (data.behavior_type == "WavePattern")
    ai.behavior = components::EnemyBehavior::kWavePattern;
  else if (data.behavior_type == "ChasePlayer")
    ai.behavior = components::EnemyBehavior::kChasePlayer;
  else if (data.behavior_type == "Straight")
    ai.behavior = components::EnemyBehavior::kStraight;
  else {
    std::cerr << "Warning: Unknown behavior type '" << data.behavior_type
              << "' for enemy '" << data.name << "'. Defaulting to Straight."
              << std::endl;
    ai.behavior = components::EnemyBehavior::kStraight;
  }

  if (config.use_custom_behavior) ai.behavior = config.custom_behavior;

  ai.speed = config.custom_speed > 0.0f ? config.custom_speed : data.speed;
  ai.detection_range = data.detection_range;
  ai.wave_amplitude = data.wave_amplitude;
  ai.wave_frequency = data.wave_frequency;

  if (ai.behavior == components::EnemyBehavior::kPatrol) {
    auto &w = GameConfig::Get().GetWorld();
    ai.patrol_min = engine::math::Vector2f{w.patrol_min_x, w.patrol_min_y};
    ai.patrol_max = engine::math::Vector2f{w.patrol_max_x, w.patrol_max_y};
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
  sprite.layer = kEnemyLayer;
  sprite.visible = true;
  sprite.tint.r = kEnemyTintR;
  sprite.tint.g = kEnemyTintG;
  sprite.tint.b = kEnemyTintB;
  sprite.tint.a = kEnemyTintA;

  registry.AddComponent<components::SpriteComponent>(enemy, std::move(sprite));

  if (data.can_shoot) {
    components::WeaponComponent weapon;
    try {
      const auto &m_data = GameConfig::Get().GetMissile("EnemyMissile");
      weapon.projectile_data.name = m_data.name;
      weapon.projectile_data.damage = m_data.damage;
      weapon.projectile_data.fire_rate = data.fire_rate;
      weapon.projectile_data.lifetime_seconds = m_data.lifetime_seconds;
      weapon.projectile_data.sprite_width = m_data.sprite_width;
      weapon.projectile_data.sprite_height = m_data.sprite_height;
      weapon.projectile_data.hitbox_scale = m_data.hitbox_scale;
      weapon.projectile_data.texture_path = m_data.texture_path;
      weapon.projectile_data.tint_color = m_data.tint_color;

      weapon.set_unlimited_ammo();
      weapon.is_trigger_held = true;
      weapon.cooldown_remaining = engine::time::TimeDelta::from_seconds(0.0f);

      weapon.projectile_data.speed = m_data.speed;
      weapon.faction = ProjectileFaction::kEnemy;

      registry.AddComponent<components::WeaponComponent>(enemy,
                                                         std::move(weapon));
    } catch (const std::exception &e) {
      std::cerr << "Error loading enemy missile config: " << e.what()
                << std::endl;
    } catch (...) {
      std::cerr << "Unknown error loading enemy missile config." << std::endl;
    }
  }

  if (config.drops_powerup) {
    registry.EmplaceComponent<components::DropsPowerupComponent>(enemy);
  }

  registry.EmplaceComponent<engine::ecs::TagComponent>(enemy, "Enemy");

  return enemy;
}

engine::ecs::EntityId EnemyBuilder::Create(
    engine::ecs::Registry &registry, EnemyType type,
    const engine::math::Vector2f &spawn_position) {
  EnemySpawnConfig config;
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