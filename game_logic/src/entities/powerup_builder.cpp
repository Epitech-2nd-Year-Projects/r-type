#include "game_logic/entities/powerup_builder.h"

#include "engine/ecs/components/bounding_box_component.h"
#include "engine/ecs/components/position_component.h"
#include "engine/ecs/components/tag_component.h"
#include "engine/ecs/components/velocity_component.h"
#include "game_logic/components/powerup_component.h"
#include "game_logic/components/sprite_component.h"
#include "game_logic/constants.h"

namespace game_logic::entities {

engine::ecs::EntityId PowerupBuilder::Create(
    engine::ecs::Registry &registry, const engine::math::Vector2f &position,
    const PowerupConfig &config) {
  engine::ecs::EntityId powerup = registry.SpawnEntity();

  registry.EmplaceComponent<engine::ecs::PositionComponent>(powerup, position);

  registry.EmplaceComponent<engine::ecs::VelocityComponent>(
      powerup, kPowerupDriftSpeed, 0.0f);

  registry.EmplaceComponent<engine::ecs::BoundingBoxComponent>(
      powerup, 0.0f, 0.0f, config.sprite_width, config.sprite_height);

  components::SpriteComponent sprite;
  sprite.texture_path = config.texture_path;
  sprite.source_rect = engine::math::RectF(0.0f, 0.0f, config.sprite_width,
                                           config.sprite_height);
  sprite.layer = kEnemyLayer;
  sprite.visible = true;
  registry.AddComponent<components::SpriteComponent>(powerup,
                                                     std::move(sprite));

  components::PowerupComponent powerup_comp;
  powerup_comp.type = config.type;
  powerup_comp.value = config.value;

  registry.AddComponent<components::PowerupComponent>(powerup,
                                                      std::move(powerup_comp));
  registry.EmplaceComponent<engine::ecs::TagComponent>(powerup, "Powerup");

  return powerup;
}

}  // namespace game_logic::entities
