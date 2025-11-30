#include "game_logic/entities/obstacle_builder.h"

#include "engine/ecs/component.h"
#include "game_logic/components.h"
#include "game_logic/entities/obstacle_data.h"

namespace game_logic::entities {

engine::ecs::EntityId ObstacleBuilder::Create(engine::ecs::Registry& registry,
                                              const ObstacleConfig& config) {
  engine::ecs::EntityId obstacle = registry.SpawnEntity();

  registry.EmplaceComponent<engine::ecs::PositionComponent>(obstacle,
                                                            config.position);

  registry.EmplaceComponent<engine::ecs::BoundingBoxComponent>(
      obstacle, 0.0f, 0.0f, config.width, config.height);

  components::SpriteComponent sprite;

  const ObstacleArchetypeData* archetype = nullptr;

  if (config.custom_texture != nullptr) {
    sprite.texture_path = config.custom_texture;
  } else if (config.type == ObstacleType::kIndestructible) {
    archetype = &kWallData;
    sprite.texture_path = archetype->texture_path;
  } else {
    archetype = &kDestructibleBarrierData;
    sprite.texture_path = archetype->texture_path;
  }

  if (archetype != nullptr) {
    sprite.tint.r = static_cast<std::uint8_t>(archetype->tint_color.r * 255.0f);
    sprite.tint.g = static_cast<std::uint8_t>(archetype->tint_color.g * 255.0f);
    sprite.tint.b = static_cast<std::uint8_t>(archetype->tint_color.b * 255.0f);
    sprite.tint.a = static_cast<std::uint8_t>(archetype->tint_color.a * 255.0f);
  }

  sprite.source_rect =
      engine::math::RectF(0.0f, 0.0f, config.width, config.height);
  sprite.layer = 3;
  sprite.visible = true;

  registry.AddComponent<components::SpriteComponent>(obstacle,
                                                     std::move(sprite));

  if (config.type == ObstacleType::kDestructible) {
    registry.EmplaceComponent<components::HealthComponent>(obstacle,
                                                           config.health);

    registry.EmplaceComponent<components::ScoreValueComponent>(
        obstacle, config.score_value);
  }

  registry.EmplaceComponent<engine::ecs::TagComponent>(obstacle, "Obstacle");

  return obstacle;
}

engine::ecs::EntityId ObstacleBuilder::CreateWall(
    engine::ecs::Registry& registry, const engine::math::Vector2f& position,
    float width, float height) {
  ObstacleConfig config;
  config.type = ObstacleType::kIndestructible;
  config.position = position;
  config.width = width;
  config.height = height;

  return Create(registry, config);
}

engine::ecs::EntityId ObstacleBuilder::CreateDestructibleBarrier(
    engine::ecs::Registry& registry, const engine::math::Vector2f& position,
    float width, float height, std::uint32_t health) {
  ObstacleConfig config;
  config.type = ObstacleType::kDestructible;
  config.position = position;
  config.width = width;
  config.height = height;
  config.health = health;
  config.score_value = kDestructibleBarrierData.score_value;

  return Create(registry, config);
}

}  // namespace game_logic::entities