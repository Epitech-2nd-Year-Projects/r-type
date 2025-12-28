#include "game_logic/systems/health_system.h"

#include <vector>

#include "engine/ecs/components/position_component.h"
#include "engine/ecs/indexed_zipper.h"
#include "engine/ecs/registry.h"
#include "engine/ecs/zipper.h"
#include "engine/scripting/prefab_factory.h"
#include "engine/util/logging.h"
#include "game_logic/components/health_component.h"
#include "game_logic/components/player_component.h"
#include "game_logic/components/powerup_drop_component.h"
#include "game_logic/components/score_value_component.h"
#include "game_logic/game_config.h"
#include "game_logic/game_instance.h"

namespace game_logic::systems {

void HealthSystem::Update(engine::ecs::Registry &registry,
                          engine::time::TimeDelta) {
  auto &logger = engine::util::Logger::Default();
  auto &healths = registry.GetComponents<components::HealthComponent>();
  auto &players = registry.GetComponents<components::PlayerComponent>();
  auto &scores_values =
      registry.GetComponents<components::ScoreValueComponent>();
  auto &positions = registry.GetComponents<engine::ecs::PositionComponent>();

  std::vector<engine::ecs::EntityId> entities_to_kill;

  for (auto [entity_idx, hp] : engine::ecs::IndexedZipper(healths)) {
    if (hp->is_alive()) {
      continue;
    }

    engine::ecs::EntityId entity = registry.EntityFromIndex(entity_idx);
    if (static_cast<size_t>(entity) < players.size() &&
        players[static_cast<size_t>(entity)].has_value()) {
      auto &player_comp = players[static_cast<size_t>(entity)].value();

      if (player_comp.lives > 0) {
        player_comp.lives--;
        game_instance_.OnPlayerDeath(player_comp.player_id, player_comp.lives);
        logger.Info("[game_logic.health] Player ", player_comp.player_id,
                    " died, lives remaining: ",
                    static_cast<int>(player_comp.lives), " respawning");
        hp->current_health = hp->max_health;
        if (static_cast<size_t>(entity) < positions.size() &&
            positions[static_cast<size_t>(entity)].has_value()) {
          auto &pos_comp = positions[static_cast<size_t>(entity)].value();
          pos_comp.position = {
              HealthSystem::kRespawnBaseX +
                  HealthSystem::kRespawnSlotOffsetX *
                      static_cast<float>(player_comp.player_slot),
              HealthSystem::kRespawnY};
        }
      } else {
        game_instance_.OnPlayerDeath(player_comp.player_id, 0);
        logger.Info("[game_logic.health] Player ", player_comp.player_id,
                    " died permanently game over");
        entities_to_kill.push_back(entity);
      }
      continue;
    }

    if (static_cast<size_t>(entity) < scores_values.size() &&
        scores_values[static_cast<size_t>(entity)].has_value()) {
      auto &score_value = scores_values[static_cast<size_t>(entity)].value();

      if (!score_value.claimed && hp->last_attacker_id.has_value()) {
        std::uint32_t attacker_id = hp->last_attacker_id.value();
        if (static_cast<size_t>(attacker_id) < players.size() &&
            players[static_cast<size_t>(attacker_id)].has_value()) {
          auto &attacker_player =
              players[static_cast<size_t>(attacker_id)].value();
          attacker_player.score += score_value.points;
          score_value.claimed = true;
        }
      }
    }
    entities_to_kill.push_back(entity);

    auto &drops = registry.GetComponents<components::DropsPowerupComponent>();
    if (static_cast<size_t>(entity) < drops.size() &&
        drops[static_cast<size_t>(entity)].has_value()) {
      try {
        if (static_cast<size_t>(entity) < positions.size() &&
            positions[static_cast<size_t>(entity)].has_value()) {
          auto &pos = positions[static_cast<size_t>(entity)].value();
          auto powerup_entity = prefab_factory_.Spawn(registry, "HealthPotion");
          if (powerup_entity) {
            registry.EmplaceComponent<engine::ecs::PositionComponent>(
                *powerup_entity, pos.position.x, pos.position.y);
          }
        }
      } catch (const std::exception &e) {
        logger.Error("[game_logic.health] Failed to spawn powerup: ", e.what());
      }
    }
  }
  for (auto entity : entities_to_kill) {
    registry.KillEntity(entity);
  }
}

}  // namespace game_logic::systems
