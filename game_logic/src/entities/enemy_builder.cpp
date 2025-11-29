#include "game_logic/entities/enemy_builder.h"

#include "engine/ecs/component.h"
#include "game_logic/components.h"

namespace game_logic::entities {

engine::ecs::EntityId EnemyBuilder::Create(engine::ecs::Registry& registry,
                                           const EnemyConfig& config) {
  engine::ecs::EntityId enemy = registry.SpawnEntity();
  registry.EmplaceComponent<engine::ecs::PositionComponent>(
      enemy, config.spawn_position);
  engine::math::Vector2f velocity = config.initial_velocity;

  if (velocity.x == 0.0f && velocity.y == 0.0f) {
    velocity = GetDefaultVelocity(config.type);
  }
  registry.EmplaceComponent<engine::ecs::VelocityComponent>(enemy, velocity);
  std::uint32_t health = config.custom_health > 0
                             ? config.custom_health
                             : GetDefaultHealth(config.type);
  registry.EmplaceComponent<components::HealthComponent>(enemy, health);
  components::AIComponent ai;
  ai.behavior = config.use_custom_behavior ? config.custom_behavior
                                           : GetDefaultBehavior(config.type);
  ai.speed = config.custom_speed > 0.0f ? config.custom_speed
                                        : GetDefaultSpeed(config.type);
  ai.detection_range = 0.0f;

  if (ai.behavior == components::EnemyBehavior::kWavePattern) {
    ai.wave_amplitude = 50.0f;
    ai.wave_frequency = 2.0f;
  }
  if (ai.behavior == components::EnemyBehavior::kPatrol) {
    ai.patrol_min = engine::math::Vector2f{0.0f, 100.0f};
    ai.patrol_max = engine::math::Vector2f{800.0f, 500.0f};
  }

  registry.AddComponent<components::AIComponent>(enemy, std::move(ai));
  std::uint32_t score = config.custom_score > 0 ? config.custom_score
                                                : GetDefaultScore(config.type);
  registry.EmplaceComponent<components::ScoreValueComponent>(enemy, score);
  engine::math::Vector2f sprite_dims = GetSpriteDimensions(config.type);
  engine::math::Vector2f hitbox_dims = GetHitboxDimensions(config.type);
  const float hitbox_offset_x = (sprite_dims.x - hitbox_dims.x) / 2.0f;
  const float hitbox_offset_y = (sprite_dims.y - hitbox_dims.y) / 2.0f;

  registry.EmplaceComponent<engine::ecs::BoundingBoxComponent>(
      enemy, hitbox_offset_x, hitbox_offset_y, hitbox_dims.x, hitbox_dims.y);

  components::SpriteComponent sprite;
  sprite.texture_path = GetTexturePath(config.type);
  sprite.source_rect =
      engine::math::RectF(0.0f, 0.0f, sprite_dims.x, sprite_dims.y);
  sprite.layer = 5;
  sprite.visible = true;
  sprite.tint.r = 255;
  sprite.tint.g = 80;
  sprite.tint.b = 80;
  sprite.tint.a = 255;
  registry.AddComponent<components::SpriteComponent>(enemy, std::move(sprite));
  registry.EmplaceComponent<engine::ecs::TagComponent>(enemy, "Enemy");
  return enemy;
}

engine::ecs::EntityId EnemyBuilder::Create(
    engine::ecs::Registry& registry, EnemyType type,
    const engine::math::Vector2f& spawn_position) {
  EnemyConfig config;
  config.type = type;
  config.spawn_position = spawn_position;

  return Create(registry, config);
}

engine::ecs::EntityId EnemyBuilder::CreateScout(
    engine::ecs::Registry& registry,
    const engine::math::Vector2f& spawn_position) {
  return Create(registry, EnemyType::kScout, spawn_position);
}

engine::ecs::EntityId EnemyBuilder::CreateBomber(
    engine::ecs::Registry& registry,
    const engine::math::Vector2f& spawn_position) {
  return Create(registry, EnemyType::kBomber, spawn_position);
}

engine::ecs::EntityId EnemyBuilder::CreateTank(
    engine::ecs::Registry& registry,
    const engine::math::Vector2f& spawn_position) {
  return Create(registry, EnemyType::kTank, spawn_position);
}

engine::math::Vector2f EnemyBuilder::GetDefaultVelocity(EnemyType type) {
  switch (type) {
    case EnemyType::kScout:
      return {-kScoutSpeed, 0.0f};
    case EnemyType::kBomber:
      return {-kBomberSpeed, 0.0f};
    case EnemyType::kTank:
      return {-kTankSpeed, 0.0f};
    default:
      return {-100.0f, 0.0f};
  }
}

engine::math::Vector2f EnemyBuilder::GetSpriteDimensions(EnemyType type) {
  switch (type) {
    case EnemyType::kScout:
      return {kScoutWidth, kScoutHeight};
    case EnemyType::kBomber:
      return {kBomberWidth, kBomberHeight};
    case EnemyType::kTank:
      return {kTankWidth, kTankHeight};
    default:
      return {32.0f, 32.0f};
  }
}

engine::math::Vector2f EnemyBuilder::GetHitboxDimensions(EnemyType type) {
  switch (type) {
    case EnemyType::kScout:
      return {kScoutHitboxWidth, kScoutHitboxHeight};
    case EnemyType::kBomber:
      return {kBomberHitboxWidth, kBomberHitboxHeight};
    case EnemyType::kTank:
      return {kTankHitboxWidth, kTankHitboxHeight};
    default:
      return {28.0f, 28.0f};
  }
}

const char* EnemyBuilder::GetTexturePath(EnemyType type) {
  switch (type) {
    case EnemyType::kScout:
      return kScoutTexturePath;
    case EnemyType::kBomber:
      return kBomberTexturePath;
    case EnemyType::kTank:
      return kTankTexturePath;
    default:
      return kScoutTexturePath;
  }
}

std::uint32_t EnemyBuilder::GetDefaultHealth(EnemyType type) {
  switch (type) {
    case EnemyType::kScout:
      return kScoutHealth;
    case EnemyType::kBomber:
      return kBomberHealth;
    case EnemyType::kTank:
      return kTankHealth;
    default:
      return 50;
  }
}

float EnemyBuilder::GetDefaultSpeed(EnemyType type) {
  switch (type) {
    case EnemyType::kScout:
      return kScoutSpeed;
    case EnemyType::kBomber:
      return kBomberSpeed;
    case EnemyType::kTank:
      return kTankSpeed;
    default:
      return 100.0f;
  }
}

components::EnemyBehavior EnemyBuilder::GetDefaultBehavior(EnemyType type) {
  switch (type) {
    case EnemyType::kScout:
      return components::EnemyBehavior::kStraight;
    case EnemyType::kBomber:
      return components::EnemyBehavior::kWavePattern;
    case EnemyType::kTank:
      return components::EnemyBehavior::kPatrol;
    default:
      return components::EnemyBehavior::kStraight;
  }
}

std::uint32_t EnemyBuilder::GetDefaultScore(EnemyType type) {
  switch (type) {
    case EnemyType::kScout:
      return kScoutScore;
    case EnemyType::kBomber:
      return kBomberScore;
    case EnemyType::kTank:
      return kTankScore;
    default:
      return 100;
  }
}

}  // namespace game_logic::entities