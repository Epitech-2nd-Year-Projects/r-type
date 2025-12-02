#include "game_logic/game_instance.h"

#include <algorithm>

#include "engine/ecs/component.h"
#include "engine/ecs/indexed_zipper.h"
#include "engine/ecs/systems/lifetime_system.h"
#include "engine/ecs/systems/movement_system.h"
#include "game_logic/components.h"
#include "game_logic/entities/player_builder.h"

namespace {

void ApplyInputToVelocity(game_logic::GameInstance::InputEventType type,
                          game_logic::GameInstance::InputState& state,
                          engine::ecs::VelocityComponent& velocity,
                          float speed) {
  using InputType = game_logic::GameInstance::InputEventType;
  auto& v = velocity.velocity;

  switch (type) {
    case InputType::kMoveLeftPressed:
      state.move_left = true;
      break;
    case InputType::kMoveLeftReleased:
      state.move_left = false;
      break;
    case InputType::kMoveRightPressed:
      state.move_right = true;
      break;
    case InputType::kMoveRightReleased:
      state.move_right = false;
      break;
    case InputType::kMoveUpPressed:
      state.move_up = true;
      break;
    case InputType::kMoveUpReleased:
      state.move_up = false;
      break;
    case InputType::kMoveDownPressed:
      state.move_down = true;
      break;
    case InputType::kMoveDownReleased:
      state.move_down = false;
      break;
  }

  float x = 0.0f;
  if (state.move_left && !state.move_right) {
    x = -speed;
  } else if (state.move_right && !state.move_left) {
    x = speed;
  }

  float y = 0.0f;
  if (state.move_up && !state.move_down) {
    y = -speed;
  } else if (state.move_down && !state.move_up) {
    y = speed;
  }

  v.x = x;
  v.y = y;
}
}  // namespace

namespace game_logic {

GameInstance::GameInstance(std::uint32_t room_id, std::uint32_t max_players)
    : room_id_(room_id),
      max_players_(max_players),
      registry_(std::make_unique<engine::ecs::Registry>()),
      game_state_(),
      player_names_(),
      player_entities_(),
      pending_inputs_(),
      is_started_(false) {
  game_state_.room_id = room_id_;
  RegisterComponents();
  RegisterSystems();
}

GameInstance::~GameInstance() {
  if (is_started_) {
    Shutdown();
  }
}

void GameInstance::Start() {
  if (is_started_) {
    return;
  }

  InitializeGame();
  game_state_.is_running = true;
  game_state_.is_game_over = false;
  game_state_.current_level = 1;
  game_state_.current_wave = 1;
  is_started_ = true;
}

void GameInstance::Update(engine::time::TimeDelta dt) {
  if (!game_state_.is_running) {
    return;
  }

  registry_->UpdateSystems(dt);
  UpdateGameState();
}

void GameInstance::Shutdown() {
  if (!is_started_) {
    return;
  }

  game_state_.is_running = false;
  registry_->ClearSystems();
  player_names_.clear();
  player_entities_.clear();
  pending_inputs_.clear();
  game_state_.active_player_ids.clear();
  game_state_.player_scores.clear();
  is_started_ = false;
}

void GameInstance::AddPlayer(std::uint32_t player_id,
                             std::string_view player_name) {
  if (player_names_.size() >= max_players_) return;
  if (player_names_.find(player_id) != player_names_.end()) return;

  player_names_.emplace(player_id, std::string(player_name));
  game_state_.active_player_ids.push_back(player_id);

  PlayerScore score(player_id, std::string(player_name));
  game_state_.player_scores.push_back(score);
}

void GameInstance::RemovePlayer(std::uint32_t player_id) {
  auto name_it = player_names_.find(player_id);
  if (name_it == player_names_.end()) {
    return;
  }

  player_names_.erase(name_it);

  auto active_it = std::find(game_state_.active_player_ids.begin(),
                             game_state_.active_player_ids.end(), player_id);
  if (active_it != game_state_.active_player_ids.end()) {
    game_state_.active_player_ids.erase(active_it);
  }

  auto score_it = std::find_if(
      game_state_.player_scores.begin(), game_state_.player_scores.end(),
      [player_id](const PlayerScore& ps) { return ps.player_id == player_id; });
  if (score_it != game_state_.player_scores.end()) {
    game_state_.player_scores.erase(score_it);
  }
}

std::optional<engine::ecs::EntityId> GameInstance::OnPlayerJoin(
    std::uint32_t player_id, std::string_view player_name) {
  if (player_names_.size() >= max_players_) {
    return std::nullopt;
  }
  if (player_names_.find(player_id) != player_names_.end()) {
    return std::nullopt;
  }

  AddPlayer(player_id, player_name);

  std::uint8_t player_slot =
      static_cast<std::uint8_t>(game_state_.active_player_ids.size() - 1);

  engine::math::Vector2f spawn_position(
      100.0f + 50.0f * static_cast<float>(player_slot), 300.0f);

  engine::ecs::EntityId entity = entities::PlayerBuilder::Create(
      *registry_, player_id, room_id_, player_slot, spawn_position);

  player_entities_.insert_or_assign(player_id, entity);
  player_input_states_.insert_or_assign(player_id, InputState{});

  return entity;
}

void GameInstance::OnPlayerLeave(std::uint32_t player_id) {
  auto entity_it = player_entities_.find(player_id);
  if (entity_it != player_entities_.end()) {
    registry_->KillEntity(entity_it->second);
    player_entities_.erase(entity_it);
  }

  RemovePlayer(player_id);

  pending_inputs_.erase(
      std::remove_if(pending_inputs_.begin(), pending_inputs_.end(),
                     [player_id](const QueuedInputEvent& evt) {
                       return evt.player_id == player_id;
                     }),
      pending_inputs_.end());

  player_input_states_.erase(player_id);
}

void GameInstance::OnPlayerInput(std::uint32_t player_id,
                                 InputEventType input_type) {
  if (player_entities_.find(player_id) == player_entities_.end()) {
    return;
  }

  QueuedInputEvent evt;
  evt.player_id = player_id;
  evt.type = input_type;
  pending_inputs_.push_back(evt);
}

engine::ecs::Registry& GameInstance::World() { return *registry_; }

const engine::ecs::Registry& GameInstance::World() const { return *registry_; }

const GameState& GameInstance::State() const { return game_state_; }

GameState& GameInstance::State() { return game_state_; }

bool GameInstance::IsRunning() const { return game_state_.is_running; }

bool GameInstance::IsFinished() const { return game_state_.is_game_over; }

std::uint32_t GameInstance::RoomId() const { return room_id_; }

std::uint32_t GameInstance::MaxPlayers() const { return max_players_; }

std::uint32_t GameInstance::ActivePlayerCount() const {
  return static_cast<std::uint32_t>(player_names_.size());
}

void GameInstance::RegisterComponents() {
  registry_->RegisterComponent<engine::ecs::PositionComponent>();
  registry_->RegisterComponent<engine::ecs::VelocityComponent>();
  registry_->RegisterComponent<engine::ecs::TransformComponent>();
  registry_->RegisterComponent<engine::ecs::BoundingBoxComponent>();
  registry_->RegisterComponent<engine::ecs::CircleColliderComponent>();
  registry_->RegisterComponent<engine::ecs::LifetimeComponent>();
  registry_->RegisterComponent<engine::ecs::TagComponent>();

  registry_->RegisterComponent<components::PlayerComponent>();
  registry_->RegisterComponent<components::HealthComponent>();
  registry_->RegisterComponent<components::WeaponComponent>();
  registry_->RegisterComponent<components::AIComponent>();
  registry_->RegisterComponent<components::SpriteComponent>();
  registry_->RegisterComponent<components::AnimationComponent>();
  registry_->RegisterComponent<components::ScoreValueComponent>();
  registry_->RegisterComponent<components::PowerupComponent>();
  registry_->RegisterComponent<components::DamageableComponent>();
}

void GameInstance::RegisterSystems() {
  registry_->AddSystem<components::PlayerComponent,
                       engine::ecs::VelocityComponent>(
      [this](
          engine::ecs::Registry&,
          engine::ecs::SparseArray<components::PlayerComponent>& players,
          engine::ecs::SparseArray<engine::ecs::VelocityComponent>& velocities,
          engine::time::TimeDelta) {
        if (pending_inputs_.empty()) {
          return;
        }

        for (const QueuedInputEvent& evt : pending_inputs_) {
          auto entity_it = player_entities_.find(evt.player_id);
          if (entity_it == player_entities_.end()) {
            continue;
          }

          auto input_it = player_input_states_.find(evt.player_id);
          if (input_it == player_input_states_.end()) {
            continue;
          }

          std::size_t index = static_cast<std::size_t>(entity_it->second);
          if (index >= players.size() || index >= velocities.size()) {
            continue;
          }

          auto& player_opt = players[index];
          auto& velocity_opt = velocities[index];
          if (!player_opt.has_value() || !velocity_opt.has_value()) {
            continue;
          }

          auto& velocity = velocity_opt.value();
          auto& input_state = input_it->second;

          ApplyInputToVelocity(evt.type, input_state, velocity,
                               kInputMoveSpeed);
        }

        pending_inputs_.clear();
      },
      engine::ecs::SystemType::Variable, engine::ecs::kHighPriority);

  registry_->AddSystem<engine::ecs::PositionComponent,
                       engine::ecs::VelocityComponent>(
      engine::ecs::MovementSystem::Update, engine::ecs::SystemType::Fixed,
      engine::ecs::kDefaultPriority);

  registry_->AddSystem<engine::ecs::LifetimeComponent>(
      engine::ecs::LifetimeSystem::Update, engine::ecs::SystemType::Variable,
      engine::ecs::kDefaultPriority);
}

void GameInstance::InitializeGame() {}

void GameInstance::UpdateGameState() {
  auto& player_components =
      registry_->GetComponents<components::PlayerComponent>();

  for (auto&& [idx, player_comp] :
       engine::ecs::IndexedZipper(player_components)) {
    std::uint32_t player_id = player_comp.value().player_id;

    for (auto& player_score : game_state_.player_scores) {
      if (player_score.player_id == player_id) {
        player_score.score = player_comp.value().score;
        player_score.lives = player_comp.value().lives;
        player_score.is_alive = player_comp.value().lives > 0;
        break;
      }
    }
  }

  bool all_dead = true;
  for (const auto& ps : game_state_.player_scores) {
    if (ps.is_alive) {
      all_dead = false;
      break;
    }
  }
  if (all_dead && !game_state_.player_scores.empty()) {
    game_state_.is_game_over = true;
    game_state_.is_running = false;
  }
}

}  // namespace game_logic
