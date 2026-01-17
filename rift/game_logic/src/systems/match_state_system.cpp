#include "rift/systems/match_state_system.h"

#include "engine/ecs/components/position_component.h"
#include "engine/ecs/components/velocity_component.h"
#include "engine/ecs/indexed_zipper.h"
#include "rift/arena_constants.h"
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

    for (auto [idx, health, fighter] :
         engine::ecs::IndexedZipper(healths, fighters)) {
      if (fighter->slot == 0) {
        fighter->rounds_won = state.player1_rounds_won;
      } else {
        fighter->rounds_won = state.player2_rounds_won;
      }
    }

    if (state.player1_rounds_won >= kRoundsToWin ||
        state.player2_rounds_won >= kRoundsToWin) {
      state.match_over = true;
    } else {
      state.round_number++;
      state.round_timer_ms = 0;

      auto& positions = registry.GetComponents<engine::ecs::PositionComponent>();
      auto& velocities = registry.GetComponents<engine::ecs::VelocityComponent>();
      auto& combats = registry.GetComponents<components::CombatStateComponent>();
      auto& attacks = registry.GetComponents<components::AttackComponent>();
      auto& staminas = registry.GetComponents<components::StaminaComponent>();

      for (auto [idx, health, fighter] :
           engine::ecs::IndexedZipper(healths, fighters)) {
        health->current_health = health->max_health;
        health->invulnerable = false;

        if (idx < positions.size() && positions[idx].has_value()) {
          const float spawn_x = fighter->slot == 0
                                    ? ArenaConstants::kPlayer1SpawnX
                                    : ArenaConstants::kPlayer2SpawnX;
          positions[idx]->position.x = spawn_x;
          positions[idx]->position.y = ArenaConstants::kGroundY;
        }

        if (idx < velocities.size() && velocities[idx].has_value()) {
          velocities[idx]->velocity.x = 0.0f;
          velocities[idx]->velocity.y = 0.0f;
        }

        if (idx < combats.size() && combats[idx].has_value()) {
          combats[idx]->state = components::CombatState::kIdle;
          combats[idx]->state_timer_ms = 0;
          combats[idx]->stun_duration_ms = 0;
        }

        if (idx < attacks.size() && attacks[idx].has_value()) {
          attacks[idx]->Reset();
        }

        if (idx < staminas.size() && staminas[idx].has_value()) {
          staminas[idx]->current_stamina = staminas[idx]->max_stamina;
        }

        fighter->facing_right = (fighter->slot == 0);
      }
    }
  }
}

}  // namespace rift::systems
