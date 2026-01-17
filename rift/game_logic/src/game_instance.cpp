#include "rift/game_instance.h"

#include "engine/ecs/components/position_component.h"
#include "engine/ecs/components/tag_component.h"
#include "engine/ecs/components/velocity_component.h"
#include "rift/arena_constants.h"
#include "rift/components/fighter_component.h"
#include "rift/systems/attack_system.h"
#include "rift/systems/block_system.h"
#include "rift/systems/combat_state_system.h"
#include "rift/systems/damage_system.h"
#include "rift/systems/dodge_system.h"
#include "rift/systems/fighter_input_system.h"
#include "rift/systems/fighter_movement_system.h"
#include "rift/systems/hit_detection_system.h"
#include "rift/systems/match_state_system.h"
#include "rift/systems/stamina_system.h"

namespace rift {

namespace {
constexpr float kArenaWidth = ArenaConstants::kArenaWidth;
constexpr float kArenaCenter = kArenaWidth / 2.0f;
constexpr float kSpawnOffset = 300.0f;
constexpr float kFighterSpeed = 150.0f;
constexpr float kGroundY = ArenaConstants::kGroundY;
}  // namespace

GameInstance::GameInstance(std::uint32_t room_id, std::uint32_t max_players)
    : room_id_(room_id),
      max_players_(max_players),
      registry_(std::make_unique<engine::ecs::Registry>()) {
  game_state_.room_id = room_id;
  RegisterComponents();
  RegisterSystems();
}

GameInstance::~GameInstance() { Shutdown(); }

void GameInstance::RegisterComponents() {
  registry_->RegisterComponent<engine::ecs::PositionComponent>();
  registry_->RegisterComponent<engine::ecs::VelocityComponent>();
  registry_->RegisterComponent<engine::ecs::TagComponent>();
  registry_->RegisterComponent<components::FighterComponent>();
  registry_->RegisterComponent<components::HealthComponent>();
  registry_->RegisterComponent<components::StaminaComponent>();
  registry_->RegisterComponent<components::CombatStateComponent>();
  registry_->RegisterComponent<components::AttackComponent>();
  registry_->RegisterComponent<components::HurtboxComponent>();
  registry_->RegisterComponent<components::HitboxComponent>();
  registry_->RegisterComponent<components::BlockComponent>();
  registry_->RegisterComponent<components::DodgeComponent>();
}

void GameInstance::RegisterSystems() {
  systems_.push_back(std::make_unique<systems::FighterInputSystem>(*this));
  systems_.push_back(std::make_unique<systems::StaminaSystem>());
  systems_.push_back(std::make_unique<systems::BlockSystem>());
  systems_.push_back(std::make_unique<systems::DodgeSystem>());
  systems_.push_back(std::make_unique<systems::AttackSystem>());
  systems_.push_back(std::make_unique<systems::FighterMovementSystem>());
  systems_.push_back(std::make_unique<systems::CombatStateSystem>());
  systems_.push_back(std::make_unique<systems::HitDetectionSystem>());
  systems_.push_back(std::make_unique<systems::DamageSystem>(*this));
  systems_.push_back(std::make_unique<systems::MatchStateSystem>(*this));
}

void GameInstance::Start() {
  if (is_started_) return;
  is_started_ = true;
  game_state_.round_number = 1;
  game_state_.player1_rounds_won = 0;
  game_state_.player2_rounds_won = 0;
  game_state_.match_over = false;
  game_state_.round_timer_ms = 0;
  InitializeGame();
}

void GameInstance::InitializeGame() {
  for (auto& [player_id, entity_id] : player_entities_) {
    auto& positions = registry_->GetComponents<engine::ecs::PositionComponent>();
    auto& healths = registry_->GetComponents<components::HealthComponent>();
    auto& fighters = registry_->GetComponents<components::FighterComponent>();
    auto& staminas = registry_->GetComponents<components::StaminaComponent>();
    auto& combats = registry_->GetComponents<components::CombatStateComponent>();
    auto& attacks = registry_->GetComponents<components::AttackComponent>();

    if (entity_id < positions.size() && positions[entity_id].has_value()) {
      auto& fighter = fighters[entity_id];
      if (fighter.has_value()) {
        float spawn_x = fighter->slot == 0
            ? kArenaCenter - kSpawnOffset
            : kArenaCenter + kSpawnOffset;
        positions[entity_id]->position.x = spawn_x;
        positions[entity_id]->position.y = kGroundY;
        fighter->facing_right = (fighter->slot == 0);
      }
    }
    if (entity_id < healths.size() && healths[entity_id].has_value()) {
      healths[entity_id]->current_health = healths[entity_id]->max_health;
      healths[entity_id]->invulnerable = false;
    }
    if (entity_id < staminas.size() && staminas[entity_id].has_value()) {
      staminas[entity_id]->current_stamina = staminas[entity_id]->max_stamina;
    }
    if (entity_id < combats.size() && combats[entity_id].has_value()) {
      combats[entity_id]->state = components::CombatState::kIdle;
      combats[entity_id]->state_timer_ms = 0;
      combats[entity_id]->stun_duration_ms = 0;
    }
    if (entity_id < attacks.size() && attacks[entity_id].has_value()) {
      attacks[entity_id]->Reset();
    }
  }
}

void GameInstance::Update(engine::time::TimeDelta dt) {
  if (!is_started_) return;

  for (auto& system : systems_) {
    system->Update(*registry_, dt);
  }

  game_state_.round_timer_ms += static_cast<std::uint32_t>(dt.as_milliseconds());
}

void GameInstance::Shutdown() {
  if (!is_started_) return;
  is_started_ = false;
  systems_.clear();
  player_entities_.clear();
  player_names_.clear();
  player_input_states_.clear();
}

std::optional<engine::ecs::EntityId> GameInstance::OnPlayerJoin(
    std::uint32_t player_id, std::string_view player_name) {
  if (player_entities_.size() >= max_players_) {
    return std::nullopt;
  }
  auto existing = player_entities_.find(player_id);
  if (existing != player_entities_.end()) {
    return existing->second;
  }

  player_names_.insert_or_assign(player_id, std::string(player_name));
  player_input_states_.insert_or_assign(player_id, InputState{});

  std::uint8_t slot = static_cast<std::uint8_t>(player_entities_.size());
  SpawnFighter(player_id, slot);

  return player_entities_.at(player_id);
}

void GameInstance::SpawnFighter(std::uint32_t player_id, std::uint8_t slot) {
  engine::ecs::EntityId entity = registry_->SpawnEntity();

  float spawn_x = slot == 0
      ? kArenaCenter - kSpawnOffset
      : kArenaCenter + kSpawnOffset;

  registry_->AddComponent<engine::ecs::PositionComponent>(
      entity, engine::ecs::PositionComponent{{spawn_x, kGroundY}});
  registry_->AddComponent<engine::ecs::VelocityComponent>(
      entity, engine::ecs::VelocityComponent{{0.0f, 0.0f}});

  components::FighterComponent fighter{};
  fighter.player_id = player_id;
  fighter.slot = slot;
  fighter.rounds_won = 0;
  fighter.facing_right = (slot == 0);
  registry_->AddComponent<components::FighterComponent>(entity, fighter);

  components::HealthComponent health{};
  health.current_health = 100;
  health.max_health = 100;
  registry_->AddComponent<components::HealthComponent>(entity, health);

  components::StaminaComponent stamina{};
  registry_->AddComponent<components::StaminaComponent>(entity, stamina);

  components::CombatStateComponent combat{};
  registry_->AddComponent<components::CombatStateComponent>(entity, combat);

  components::AttackComponent attack{};
  registry_->AddComponent<components::AttackComponent>(entity, attack);

  components::HurtboxComponent hurtbox{};
  registry_->AddComponent<components::HurtboxComponent>(entity, hurtbox);

  components::HitboxComponent hitbox{};
  registry_->AddComponent<components::HitboxComponent>(entity, hitbox);

  components::BlockComponent block{};
  registry_->AddComponent<components::BlockComponent>(entity, block);

  components::DodgeComponent dodge{};
  registry_->AddComponent<components::DodgeComponent>(entity, dodge);

  player_entities_.insert_or_assign(player_id, entity);
}

void GameInstance::OnPlayerLeave(std::uint32_t player_id) {
  auto it = player_entities_.find(player_id);
  if (it != player_entities_.end()) {
    registry_->KillEntity(it->second);
    player_entities_.erase(it);
  }
  player_names_.erase(player_id);
  player_input_states_.erase(player_id);
}

void GameInstance::OnPlayerInput(std::uint32_t player_id,
                                  InputEventType input_type) {
  auto state_it = player_input_states_.find(player_id);
  if (state_it == player_input_states_.end()) return;

  auto entity_it = player_entities_.find(player_id);
  if (entity_it == player_entities_.end()) return;

  InputState& input = state_it->second;
  engine::ecs::EntityId entity = entity_it->second;

  auto& velocities = registry_->GetComponents<engine::ecs::VelocityComponent>();
  if (entity >= velocities.size() || !velocities[entity].has_value()) return;

  auto& combat_states = registry_->GetComponents<components::CombatStateComponent>();
  if (entity >= combat_states.size() || !combat_states[entity].has_value()) return;

  auto& combat = *combat_states[entity];

  if (combat.state == components::CombatState::kStunned) {
    return;
  }

  switch (input_type) {
    case InputEventType::kMoveLeftPressed:
      input.move_left = true;
      break;
    case InputEventType::kMoveLeftReleased:
      input.move_left = false;
      break;
    case InputEventType::kMoveRightPressed:
      input.move_right = true;
      break;
    case InputEventType::kMoveRightReleased:
      input.move_right = false;
      break;
    case InputEventType::kBlockPressed:
      ProcessBlockInput(player_id, true);
      input.blocking = true;
      break;
    case InputEventType::kBlockReleased:
      ProcessBlockInput(player_id, false);
      input.blocking = false;
      break;
    case InputEventType::kDodgePressed:
      ProcessDodgeInput(player_id);
      break;
    case InputEventType::kDodgeReleased:
      break;
    case InputEventType::kLightAttackPressed:
      ProcessAttackInput(player_id, true, true);
      input.light_attack = true;
      break;
    case InputEventType::kLightAttackReleased:
      input.light_attack = false;
      break;
    case InputEventType::kHeavyAttackPressed:
      ProcessAttackInput(player_id, false, true);
      input.heavy_attack = true;
      break;
    case InputEventType::kHeavyAttackReleased:
      input.heavy_attack = false;
      break;
  }

  if (combat.state != components::CombatState::kAttacking &&
      combat.state != components::CombatState::kDodging) {
    float vx = 0.0f;
    if (input.move_left) vx -= kFighterSpeed;
    if (input.move_right) vx += kFighterSpeed;
    velocities[entity]->velocity.x = vx;
  }
}

void GameInstance::ProcessAttackInput(std::uint32_t player_id, bool light,
                                       bool pressed) {
  if (!pressed) return;

  auto entity_it = player_entities_.find(player_id);
  if (entity_it == player_entities_.end()) return;

  engine::ecs::EntityId entity = entity_it->second;

  auto& attacks = registry_->GetComponents<components::AttackComponent>();
  auto& staminas = registry_->GetComponents<components::StaminaComponent>();
  auto& combat_states = registry_->GetComponents<components::CombatStateComponent>();
  auto& velocities = registry_->GetComponents<engine::ecs::VelocityComponent>();

  if (entity >= attacks.size() || !attacks[entity].has_value()) return;
  if (entity >= staminas.size() || !staminas[entity].has_value()) return;
  if (entity >= combat_states.size() || !combat_states[entity].has_value()) return;

  auto& attack = *attacks[entity];
  auto& stamina = *staminas[entity];
  auto& combat = *combat_states[entity];

  if (attack.type != components::AttackType::kNone) return;
  if (combat.state == components::CombatState::kStunned ||
      combat.state == components::CombatState::kDodging) return;

  components::AttackType attack_type = light
      ? components::AttackType::kLight
      : components::AttackType::kHeavy;

  float cost = light
      ? components::AttackComponent::kLightStaminaCost
      : components::AttackComponent::kHeavyStaminaCost;

  if (!stamina.CanAfford(cost)) return;

  stamina.Consume(cost);
  attack.StartAttack(attack_type);
  combat.state = components::CombatState::kAttacking;
  combat.state_timer_ms = 0;

  if (entity < velocities.size() && velocities[entity].has_value()) {
    velocities[entity]->velocity.x = 0.0f;
  }
}

void GameInstance::ProcessBlockInput(std::uint32_t player_id, bool pressed) {
  auto entity_it = player_entities_.find(player_id);
  if (entity_it == player_entities_.end()) return;

  engine::ecs::EntityId entity = entity_it->second;

  auto& blocks = registry_->GetComponents<components::BlockComponent>();
  auto& combat_states = registry_->GetComponents<components::CombatStateComponent>();
  auto& attacks = registry_->GetComponents<components::AttackComponent>();

  if (entity >= blocks.size() || !blocks[entity].has_value()) return;
  if (entity >= combat_states.size() || !combat_states[entity].has_value()) return;

  auto& block = *blocks[entity];
  auto& combat = *combat_states[entity];

  if (combat.state == components::CombatState::kStunned ||
      combat.state == components::CombatState::kDodging ||
      combat.state == components::CombatState::kAttacking) {
    return;
  }

  if (entity < attacks.size() && attacks[entity].has_value()) {
    if (attacks[entity]->type != components::AttackType::kNone) return;
  }

  block.is_blocking = pressed;
}

void GameInstance::ProcessDodgeInput(std::uint32_t player_id) {
  auto entity_it = player_entities_.find(player_id);
  if (entity_it == player_entities_.end()) return;
  auto state_it = player_input_states_.find(player_id);
  if (state_it == player_input_states_.end()) return;

  engine::ecs::EntityId entity = entity_it->second;
  InputState& input = state_it->second;

  auto& dodges = registry_->GetComponents<components::DodgeComponent>();
  auto& staminas = registry_->GetComponents<components::StaminaComponent>();
  auto& combat_states = registry_->GetComponents<components::CombatStateComponent>();
  auto& fighters = registry_->GetComponents<components::FighterComponent>();

  if (entity >= dodges.size() || !dodges[entity].has_value()) return;
  if (entity >= staminas.size() || !staminas[entity].has_value()) return;
  if (entity >= combat_states.size() || !combat_states[entity].has_value()) return;
  if (entity >= fighters.size() || !fighters[entity].has_value()) return;

  auto& dodge = *dodges[entity];
  auto& stamina = *staminas[entity];
  auto& combat = *combat_states[entity];
  auto& fighter = *fighters[entity];

  if (dodge.is_dodging) return;
  if (dodge.cooldown_remaining_ms > 0.0f) return;
  if (combat.state == components::CombatState::kStunned) return;

  if (!stamina.CanAfford(dodge.stamina_cost)) return;

  stamina.Consume(dodge.stamina_cost);
  dodge.is_dodging = true;
  dodge.current_time_ms = 0.0f;

  if (input.move_left) {
    dodge.direction = -1;
  } else if (input.move_right) {
    dodge.direction = 1;
  } else {
    dodge.direction = fighter.facing_right ? 1 : -1;
  }

  combat.state = components::CombatState::kDodging;
  combat.state_timer_ms = 0;
}

void GameInstance::OnPlayerDeath(std::uint32_t player_id,
                                  std::uint8_t remaining_lives) {
  pending_deaths_.push_back({player_id, remaining_lives});
}

std::vector<GameInstance::PlayerDeathEvent>
GameInstance::ExtractPlayerDeathEvents() {
  std::vector<PlayerDeathEvent> events;
  std::swap(events, pending_deaths_);
  return events;
}

engine::ecs::Registry& GameInstance::World() { return *registry_; }

const engine::ecs::Registry& GameInstance::World() const { return *registry_; }

const GameState& GameInstance::State() const { return game_state_; }

GameState& GameInstance::State() { return game_state_; }

bool GameInstance::IsRunning() const { return is_started_ && !game_state_.match_over; }

bool GameInstance::IsFinished() const { return game_state_.match_over; }

std::uint32_t GameInstance::RoomId() const { return room_id_; }

std::uint32_t GameInstance::MaxPlayers() const { return max_players_; }

std::uint32_t GameInstance::ActivePlayerCount() const {
  return static_cast<std::uint32_t>(player_entities_.size());
}

}  // namespace rift
