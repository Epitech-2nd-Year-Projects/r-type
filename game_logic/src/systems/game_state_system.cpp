#include "game_logic/systems/game_state_system.h"

#include "engine/ecs/indexed_zipper.h"
#include "game_logic/components/player_component.h"
#include "game_logic/components/score_value_component.h"
#include "game_logic/game_instance.h"

namespace game_logic::systems {

GameStateSystem::GameStateSystem(GameInstance &game_instance)
    : game_instance_(game_instance) {}

void GameStateSystem::Update(engine::ecs::Registry &registry,
                             engine::time::TimeDelta) {
  auto &player_components =
      registry.GetComponents<components::PlayerComponent>();

  for (auto &&[idx, player_comp] :
       engine::ecs::IndexedZipper(player_components)) {
    std::uint32_t player_id = player_comp.value().player_id;

    for (auto &player_score : game_instance_.game_state_.player_scores) {
      if (player_score.player_id == player_id) {
        player_score.score = player_comp.value().score;
        player_score.lives = player_comp.value().lives;
        player_score.is_alive = player_comp.value().lives > 0;
        break;
      }
    }
  }

  if (!game_instance_.game_state_.player_scores.empty()) {
    bool all_dead = true;
    for (const auto &ps : game_instance_.game_state_.player_scores) {
      if (ps.is_alive) {
        all_dead = false;
        break;
      }
    }

    if (all_dead) {
      game_instance_.game_state_.is_game_over = true;
      game_instance_.game_state_.is_running = false;
    }
  }
}

} // namespace game_logic::systems
