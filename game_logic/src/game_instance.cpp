#include "game_logic/game_instance.h"

#include <algorithm>

#include "engine/ecs/component.h"
#include "engine/ecs/indexed_zipper.h"
#include "engine/ecs/registry.h"
#include "engine/ecs/systems/lifetime_system.h"
#include "engine/ecs/systems/movement_system.h"
#include "engine/event.h"
#include "engine/scripting/bindings.h"
#include "engine/scripting/script_engine.h"
#include "engine/time/time_delta.h"
#include "game_logic/components.h"
#include "game_logic/components/powerup_drop_component.h"
#include "game_logic/constants.h"
#include "game_logic/entities/player_builder.h"
#include "game_logic/game_config.h"
#include "game_logic/systems/ai_system.h"
#include "game_logic/systems/animation_system.h"
#include "game_logic/systems/boundary_system.h"
#include "game_logic/systems/collision_system.h"
#include "game_logic/systems/game_state_system.h"
#include "game_logic/systems/health_system.h"
#include "game_logic/systems/player_input_system.h"
#include "game_logic/systems/powerup_system.h"
#include "game_logic/systems/wave_system.h"
#include "game_logic/systems/weapon_system.h"

namespace game_logic {

GameInstance::GameInstance(std::uint32_t room_id, std::uint32_t max_players)
    : room_id_(room_id),
      max_players_(max_players),
      registry_(std::make_unique<engine::ecs::Registry>()),
      script_engine_(std::make_unique<engine::scripting::ScriptEngine>()),
      game_state_(),
      is_started_(false) {
  game_state_.room_id = room_id_;
  if (!GameConfig::Get().Load("config")) {
    std::cerr
        << "FATAL: Failed to load game config! Defaults usage is dangerous "
           "and may cause crashes."
        << std::endl;
  }

  script_engine_->Initialize();
  engine::scripting::BindRegistry(script_engine_->LuaState(), *registry_);
  engine::scripting::BindEventBus(script_engine_->LuaState(), event_bus_);

  event_bus_.Subscribe<systems::EntityCollisionEvent>(
      [this](const systems::EntityCollisionEvent &e) {
        if (!script_engine_) {
          return;
        }
        auto &lua = script_engine_->LuaState();
        sol::table data = lua.create_table();
        data["entity_a"] = e.entity_a;
        data["entity_b"] = e.entity_b;
        event_bus_.Publish(
            engine::scripting::LuaEvent{"OnCollision", std::move(data)});
      });

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
  player_input_states_.clear();
  game_state_.active_player_ids.clear();
  game_state_.player_scores.clear();
  is_started_ = false;
}

void GameInstance::AddPlayer(std::uint32_t player_id,
                             std::string_view player_name) {
  if (player_names_.size() >= max_players_) {
    return;
  }
  if (player_names_.find(player_id) != player_names_.end()) {
    return;
  }

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
      [player_id](const PlayerScore &ps) { return ps.player_id == player_id; });
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
      kPlayerSpawnBaseX + kPlayerSpawnOffsetX * static_cast<float>(player_slot),
      kPlayerSpawnY);

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
                     [player_id](const QueuedInputEvent &evt) {
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

void GameInstance::OnPlayerDeath(std::uint32_t player_id,
                                 std::uint8_t remaining_lives) {
  pending_deaths_.push_back({player_id, remaining_lives});
}

std::vector<GameInstance::PlayerDeathEvent>
GameInstance::ExtractPlayerDeathEvents() {
  std::vector<PlayerDeathEvent> events;
  events.swap(pending_deaths_);
  return events;
}

engine::ecs::Registry &GameInstance::World() { return *registry_; }

const engine::ecs::Registry &GameInstance::World() const { return *registry_; }

const GameState &GameInstance::State() const { return game_state_; }

GameState &GameInstance::State() { return game_state_; }

engine::scripting::ScriptEngine &GameInstance::ScriptEngine() {
  return *script_engine_;
}

engine::event::EventBus &GameInstance::EventBus() { return event_bus_; }

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
  registry_->RegisterComponent<components::DropsPowerupComponent>();
}

void GameInstance::RegisterSystems() {
  registry_
      ->AddSystem<components::PlayerComponent, engine::ecs::VelocityComponent,
                  components::WeaponComponent>(
          systems::PlayerInputSystem::Update, engine::ecs::SystemType::Variable,
          engine::ecs::kHighPriority, std::ref(*this));

  registry_
      ->AddSystem<engine::ecs::PositionComponent, components::WeaponComponent,
                  components::SpriteComponent>(
          systems::WeaponSystem::Update, engine::ecs::SystemType::Variable,
          engine::ecs::kDefaultPriority);

  registry_->AddSystem<engine::ecs::PositionComponent,
                       engine::ecs::VelocityComponent>(
      engine::ecs::MovementSystem::Update, engine::ecs::SystemType::Fixed,
      engine::ecs::kDefaultPriority);

  registry_->AddSystemClass(std::make_shared<systems::BoundarySystem>(),
                            engine::ecs::SystemType::Fixed,
                            engine::ecs::kDefaultPriority);

  registry_->AddSystemClass(
      std::make_shared<systems::CollisionSystem>(event_bus_),
      engine::ecs::SystemType::Fixed, engine::ecs::kDefaultPriority);

  registry_->AddSystemClass(std::make_shared<systems::AnimationSystem>(),
                            engine::ecs::SystemType::Variable,
                            engine::ecs::kDefaultPriority);

  registry_->AddSystemClass(std::make_shared<systems::AISystem>(),
                            engine::ecs::SystemType::Fixed,
                            engine::ecs::kDefaultPriority);

  registry_->AddSystemClass(std::make_shared<systems::HealthSystem>(*this),
                            engine::ecs::SystemType::Fixed,
                            engine::ecs::kDefaultPriority);

  registry_->AddSystemClass(std::make_shared<systems::WaveSystem>(*this),
                            engine::ecs::SystemType::Fixed,
                            engine::ecs::kDefaultPriority);

  registry_->AddSystemClass(std::make_shared<systems::GameStateSystem>(*this),
                            engine::ecs::SystemType::Fixed,
                            engine::ecs::kDefaultPriority);

  registry_->AddSystemClass(std::make_shared<systems::PowerupSystem>(),
                            engine::ecs::SystemType::Fixed,
                            engine::ecs::kDefaultPriority);

  registry_->AddSystem<engine::ecs::LifetimeComponent>(
      engine::ecs::LifetimeSystem::Update, engine::ecs::SystemType::Variable,
      engine::ecs::kDefaultPriority);
}

void GameInstance::InitializeGame() {}

void GameInstance::UpdateGameState() {}

}  // namespace game_logic
