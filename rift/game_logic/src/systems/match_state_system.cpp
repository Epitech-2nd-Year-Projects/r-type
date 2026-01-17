#include "rift/systems/match_state_system.h"

#include "engine/ecs/indexed_zipper.h"
#include "rift/components/fighter_component.h"
#include "rift/game_instance.h"

namespace rift::systems {

MatchStateSystem::MatchStateSystem(GameInstance& instance) : instance_(instance) {}

void MatchStateSystem::Update(engine::ecs::Registry& registry,
                              engine::time::TimeDelta dt) {
  auto& state = instance_.State();

  if (state.match_over) return;

  auto& healths = registry.GetComponents<components::HealthComponent>();
  auto& fighters = registry.GetComponents<components::FighterComponent>();

  std::uint32_t p1_health = 0;
  std::uint32_t p2_health = 0;
  bool p1_alive = false;
  bool p2_alive = false;

  for (auto [idx, health, fighter] :
       engine::ecs::IndexedZipper(healths, fighters)) {
    if (fighter->slot == 0) {
      p1_health = health->current_health;
      p1_alive = health->IsAlive();
    } else {
      p2_health = health->current_health;
      p2_alive = health->IsAlive();
    }
  }

  bool round_over = false;
  std::uint8_t winner = 0;

  if (!p1_alive) {
    round_over = true;
    winner = 1;
  } else if (!p2_alive) {
    round_over = true;
    winner = 0;
  } else if (state.round_timer_ms >= kRoundTimeLimitMs) {
    round_over = true;
    winner = (p1_health >= p2_health) ? 0 : 1;
  }

  if (round_over) {
    if (winner == 0) {
      state.player1_rounds_won++;
    } else {
      state.player2_rounds_won++;
    }

    if (state.player1_rounds_won >= kRoundsToWin ||
        state.player2_rounds_won >= kRoundsToWin) {
      state.match_over = true;
    } else {
      state.round_number++;
      state.round_timer_ms = 0;

      for (auto [idx, health, fighter] :
           engine::ecs::IndexedZipper(healths, fighters)) {
        health->current_health = health->max_health;
        health->invulnerable = false;
      }
    }
  }
}

}  // namespace rift::systems
