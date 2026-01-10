#include "rift/game_instance.h"

#include "engine/ecs/components/position_component.h"
#include "engine/ecs/components/velocity_component.h"
#include "rift/components/fighter_component.h"

namespace rift {

namespace {
constexpr float kArenaWidth = 800.0f;
constexpr float kArenaCenter = kArenaWidth / 2.0f;
constexpr float kSpawnOffset = 200.0f;
constexpr float kFighterSpeed = 150.0f;
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
  registry_->RegisterComponent<components::FighterComponent>();
  registry_->RegisterComponent<components::HealthComponent>();
  registry_->RegisterComponent<components::StaminaComponent>();
  registry_->RegisterComponent<components::CombatStateComponent>();
}

void GameInstance::RegisterSystems() {
  // Systems will be added as the game logic is expanded
}

void GameInstance::Start() {
  if (is_started_) return;
  is_started_ = true;
  game_state_.round_number = 1;
  game_state_.player1_rounds_won = 0;
  game_state_.player2_rounds_won = 0;
  game_state_.match_over = false;
  InitializeGame();
}

void GameInstance::InitializeGame() {
  // Reset fighter positions and health at round start
  for (auto& [player_id, entity_id] : player_entities_) {
    auto& positions = registry_->GetComponents<engine::ecs::PositionComponent>();
    auto& healths = registry_->GetComponents<components::HealthComponent>();
    auto& fighters = registry_->GetComponents<components::FighterComponent>();

    if (entity_id < positions.size() && positions[entity_id].has_value()) {
      auto& fighter = fighters[entity_id];
      if (fighter.has_value()) {
        float spawn_x = fighter->slot == 0
            ? kArenaCenter - kSpawnOffset
            : kArenaCenter + kSpawnOffset;
        positions[entity_id]->position.x = spawn_x;
        positions[entity_id]->position.y = 0.0f;
        fighter->facing_right = (fighter->slot == 0);
      }
    }
    if (entity_id < healths.size() && healths[entity_id].has_value()) {
      healths[entity_id]->current_health = healths[entity_id]->max_health;
    }
  }
}

void GameInstance::Update(engine::time::TimeDelta dt) {
  if (!is_started_) return;

  auto& positions = registry_->GetComponents<engine::ecs::PositionComponent>();
  auto& velocities = registry_->GetComponents<engine::ecs::VelocityComponent>();

  const float dt_sec = dt.AsSeconds();

  for (auto& [player_id, entity_id] : player_entities_) {
    if (entity_id >= positions.size() || !positions[entity_id].has_value()) {
      continue;
    }
    if (entity_id >= velocities.size() || !velocities[entity_id].has_value()) {
      continue;
    }

    auto& pos = positions[entity_id]->position;
    const auto& vel = velocities[entity_id]->velocity;

    pos.x += vel.x * dt_sec;
    pos.y += vel.y * dt_sec;

    pos.x = std::clamp(pos.x, 0.0f, kArenaWidth);
  }

  game_state_.round_timer_ms += static_cast<std::uint32_t>(dt.AsMilliseconds());
}

void GameInstance::Shutdown() {
  if (!is_started_) return;
  is_started_ = false;
  player_entities_.clear();
  player_names_.clear();
  player_input_states_.clear();
}

std::optional<engine::ecs::EntityId> GameInstance::OnPlayerJoin(
    std::uint32_t player_id, std::string_view player_name) {
  if (player_entities_.size() >= max_players_) {
    return std::nullopt;
  }
  if (player_entities_.count(player_id) > 0) {
    return player_entities_[player_id];
  }

  player_names_[player_id] = std::string(player_name);
  player_input_states_[player_id] = InputState{};

  std::uint8_t slot = static_cast<std::uint8_t>(player_entities_.size());
  SpawnFighter(player_id, slot);

  return player_entities_[player_id];
}

void GameInstance::SpawnFighter(std::uint32_t player_id, std::uint8_t slot) {
  engine::ecs::EntityId entity = registry_->CreateEntity();

  float spawn_x = slot == 0
      ? kArenaCenter - kSpawnOffset
      : kArenaCenter + kSpawnOffset;

  registry_->AddComponent<engine::ecs::PositionComponent>(
      entity, engine::ecs::PositionComponent{{spawn_x, 0.0f}});
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

  player_entities_[player_id] = entity;
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
    case InputEventType::kMoveUpPressed:
      input.blocking = true;
      break;
    case InputEventType::kMoveUpReleased:
      input.blocking = false;
      break;
    case InputEventType::kMoveDownPressed:
      input.dodging = true;
      break;
    case InputEventType::kMoveDownReleased:
      input.dodging = false;
      break;
    default:
      break;
  }

  float vx = 0.0f;
  if (input.move_left) vx -= kFighterSpeed;
  if (input.move_right) vx += kFighterSpeed;
  velocities[entity]->velocity.x = vx;
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
