#include "game_logic/systems/powerup_system.h"

#include <iostream>
#include <vector>

#include "engine/ecs/components/bounding_box_component.h"
#include "engine/ecs/components/position_component.h"
#include "engine/ecs/components/tag_component.h"
#include "engine/ecs/indexed_zipper.h"
#include "game_logic/components/health_component.h"
#include "game_logic/components/player_component.h"
#include "game_logic/components/powerup_component.h"
#include "game_logic/constants.h"

namespace game_logic::systems {

void PowerupSystem::Update(engine::ecs::Registry &registry,
                           engine::time::TimeDelta dt) {
  auto &powerups = registry.GetComponents<components::PowerupComponent>();
  auto &positions = registry.GetComponents<engine::ecs::PositionComponent>();
  auto &boxes = registry.GetComponents<engine::ecs::BoundingBoxComponent>();
  auto &tags = registry.GetComponents<engine::ecs::TagComponent>();

  std::vector<engine::ecs::EntityId> players;
  for (size_t i = 0; i < tags.size(); ++i) {
    if (tags[i].has_value() && tags[i]->tag == "Player") {
      players.push_back(registry.EntityFromIndex(i));
    }
  }

  std::vector<engine::ecs::EntityId> collected_powerups;

  for (auto &&[entity, powerup, pos, box] :
       engine::ecs::IndexedZipper(powerups, positions, boxes)) {
    if (!powerup->active) continue;

    bool collected = false;

    engine::math::RectF powerup_rect = box->bounds;
    powerup_rect.top_left_x_ += pos->position.x;
    powerup_rect.top_left_y_ += pos->position.y;

    for (auto player_id : players) {
      if (player_id >= positions.size() || !positions[player_id].has_value() ||
          player_id >= boxes.size() || !boxes[player_id].has_value()) {
        continue;
      }

      auto &p_pos = positions[player_id].value();
      auto &p_box = boxes[player_id].value();

      engine::math::RectF player_rect = p_box.bounds;
      player_rect.top_left_x_ += p_pos.position.x;
      player_rect.top_left_y_ += p_pos.position.y;

      if (powerup_rect.Intersects(player_rect)) {
        engine::ecs::EntityId entity_id = registry.EntityFromIndex(entity);
        CollectPowerup(registry, player_id, entity_id);
        collected_powerups.push_back(entity_id);
        collected = true;
        break;
      }
    }

    if (!collected && pos->position.x < kPowerupCleanupBoundary) {
      collected_powerups.push_back(registry.EntityFromIndex(entity));
    }
  }

  for (auto p : collected_powerups) {
    registry.KillEntity(p);
  }
}

void PowerupSystem::CollectPowerup(engine::ecs::Registry &registry,
                                   engine::ecs::EntityId player,
                                   engine::ecs::EntityId powerup) {
  auto &powerups = registry.GetComponents<components::PowerupComponent>();
  auto &healths = registry.GetComponents<components::HealthComponent>();

  if (powerup >= powerups.size() || !powerups[powerup].has_value()) return;
  const auto &powerup_data = powerups[powerup].value();

  switch (powerup_data.type) {
    case components::PowerupType::kHealth:
      if (player < healths.size() && healths[player].has_value()) {
        healths[player]->heal(powerup_data.value);
      }
      break;
    default:
      break;
  }
}

}  // namespace game_logic::systems
