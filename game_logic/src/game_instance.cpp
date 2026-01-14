#include "game_logic/game_instance.h"

#include <algorithm>

#include "engine/ecs/component.h"
#include "engine/ecs/components/compound_circle_collider_component.h"
#include "engine/ecs/indexed_zipper.h"
#include "engine/ecs/registry.h"
#include "engine/ecs/systems/lifetime_system.h"
#include "engine/ecs/systems/movement_system.h"
#include "engine/event.h"
#include "engine/scripting/bindings.h"
#include "engine/scripting/script_engine.h"
#include "engine/time/time_delta.h"
#include "engine/util/logging.h"
#include "game_logic/bindings.h"
#include "game_logic/components.h"
#include "game_logic/components/powerup_drop_component.h"
#include "game_logic/constants.h"
#include "game_logic/game_config.h"
#include "game_logic/prefab_binder.h"
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

GameInstance::GameInstance(std::uint32_t room_id, std::uint32_t max_players,
                           Difficulty difficulty)
    : room_id_(room_id),
      max_players_(max_players),
      difficulty_(difficulty),
      registry_(std::make_unique<engine::ecs::Registry>()),
      script_engine_(std::make_unique<engine::scripting::ScriptEngine>()),
      game_state_(),
      is_started_(false) {
  game_state_.room_id = room_id_;
  if (!GameConfig::Get().Load("config")) {
    engine::util::Logger::Default().Error(
        "[game_logic] Failed to load game config defaults may cause crashes");
  }

  script_engine_->Initialize();
  engine::scripting::BindRegistry(script_engine_->LuaState(), *registry_);
  engine::scripting::BindEventBus(script_engine_->LuaState(), event_bus_);
  BindGameComponents(script_engine_->GetPrefabFactory());
  BindRuntimeTypes(script_engine_->LuaState(),
                   script_engine_->GetPrefabFactory());

  script_engine_->LuaState().set_function(
      "SignalPlayerDeath",
      [this](std::uint32_t player_id, std::uint32_t lives) {
        this->OnPlayerDeath(player_id, static_cast<std::uint8_t>(lives));
      });

  BindGameComponents(script_engine_->GetPrefabFactory());

  std::string config_dir = GameConfig::Get().GetConfigDirectory();
  script_engine_->LoadScript(config_dir + "/prefabs/enemies.lua");
  script_engine_->LoadScript(config_dir + "/prefabs/dobkeratops.lua");
  script_engine_->LoadScript(config_dir + "/prefabs/players.lua");
  script_engine_->LoadScript(config_dir + "/prefabs/weapons.lua");
  script_engine_->LoadScript(config_dir + "/prefabs/obstacles.lua");
  script_engine_->LoadScript(config_dir + "/prefabs/powerups.lua");
  script_engine_->LoadScript(config_dir + "/behaviors/ai.lua");
  script_engine_->LoadScript(config_dir + "/behaviors/ai.lua");
  script_engine_->LoadScript(config_dir + "/behaviors/weapon_logic.lua");
  script_engine_->LoadScript(config_dir + "/behaviors/collision_logic.lua");
  script_engine_->LoadScript(config_dir + "/difficulty.lua");
  script_engine_->LoadScript(config_dir + "/behaviors/spawn_helper.lua");
  InitializeDifficultyModifiers();

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
  if (script_engine_) {
    script_engine_->Update();
  }
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

  auto opt_entity =
      script_engine_->GetPrefabFactory().Spawn(*registry_, "Player");
  if (!opt_entity) {
    engine::util::Logger::Default().Error(
        "[GameInstance] Failed to spawn Player prefab");
    return std::nullopt;
  }
  engine::util::Logger::Default().Info("[GameInstance Logic] Player Spawned");
  engine::ecs::EntityId entity = *opt_entity;
  registry_->EmplaceComponent<engine::ecs::PositionComponent>(
      entity, spawn_position.x, spawn_position.y);
  registry_->EmplaceComponent<components::ShootEventComponent>(entity);

  auto &players = registry_->GetComponents<components::PlayerComponent>();
  if (static_cast<std::size_t>(entity) < players.size() &&
      players[entity].has_value()) {
    auto &pc = players[entity].value();
    pc.player_id = player_id;
    pc.room_id = room_id_;
    pc.player_slot = player_slot;
  }

  auto &lua = script_engine_->LuaState();
  sol::optional<sol::function> apply_modifiers =
      lua["SpawnHelper"]["ApplyPlayerModifiers"];
  engine::util::Logger::Default().Info(
      "[GameInstance Logic] Checking modifiers");
  if (apply_modifiers.has_value()) {
    sol::object registry_obj = lua["registry"];
    if (registry_obj.valid()) {
      try {
        engine::util::Logger::Default().Info(
            "[GameInstance Logic] Calling Lua ApplyPlayerModifiers");
        apply_modifiers.value()(registry_obj, entity);
        engine::util::Logger::Default().Info(
            "[GameInstance Logic] Lua ApplyPlayerModifiers returned");
      } catch (const sol::error &e) {
        engine::util::Logger::Default().Error<std::string>(
            "[GameInstance] Lua error in ApplyPlayerModifiers: " +
            std::string(e.what()));
      }
    } else {
      engine::util::Logger::Default().Error(
          "[GameInstance] Lua registry global not found");
    }
  }

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

void GameInstance::OnPlayerInput(
    std::uint32_t player_id, InputEventType input_type,
    std::optional<engine::math::Vector2f> spawn_pos, float latency_s) {
  if (player_entities_.find(player_id) == player_entities_.end()) {
    return;
  }

  QueuedInputEvent evt;
  evt.player_id = player_id;
  evt.type = input_type;
  evt.spawn_pos = spawn_pos;
  evt.latency_s = latency_s;
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

Difficulty GameInstance::GetDifficulty() const { return difficulty_; }

void GameInstance::InitializeDifficultyModifiers() {
  auto &lua = script_engine_->LuaState();

  std::string_view difficulty_name = DifficultyToString(difficulty_);

  sol::optional<sol::table> settings = lua["DifficultySettings"];
  if (settings.has_value()) {
    sol::optional<sol::table> modifiers =
        settings.value()[std::string(difficulty_name)];
    if (modifiers.has_value()) {
      lua["DifficultyModifiers"] = modifiers.value();
      engine::util::Logger::Default().Info("[GameInstance] Loaded difficulty: ",
                                           difficulty_name);
      return;
    }
  }

  sol::table defaults = lua.create_table();
  defaults["enemy_speed_multiplier"] = 1.0f;
  defaults["enemy_health_multiplier"] = 1.0f;
  defaults["enemy_damage_multiplier"] = 1.0f;
  defaults["enemy_fire_rate_multiplier"] = 1.0f;
  defaults["player_health"] = 100;
  defaults["player_lives"] = 3;
  defaults["score_multiplier"] = 1.0f;
  lua["DifficultyModifiers"] = defaults;
  engine::util::Logger::Default().Warn(
      "[GameInstance] Using default difficulty modifiers");
}

void GameInstance::RegisterComponents() {
  registry_->RegisterComponent<engine::ecs::PositionComponent>();
  registry_->RegisterComponent<engine::ecs::VelocityComponent>();
  registry_->RegisterComponent<engine::ecs::TransformComponent>();
  registry_->RegisterComponent<engine::ecs::BoundingBoxComponent>();
  registry_->RegisterComponent<engine::ecs::CircleColliderComponent>();
  registry_->RegisterComponent<engine::ecs::CompoundCircleColliderComponent>();
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
  registry_->RegisterComponent<components::ShootEventComponent>();
}

void GameInstance::RegisterSystems() {
  registry_
      ->AddSystem<components::PlayerComponent, engine::ecs::VelocityComponent,
                  components::WeaponComponent, components::ShootEventComponent>(
          systems::PlayerInputSystem::Update, engine::ecs::SystemType::Variable,
          engine::ecs::kHighPriority, std::ref(*this));

  registry_->AddSystemClass(
      std::make_shared<systems::WeaponSystem>(*script_engine_),
      engine::ecs::SystemType::Variable, engine::ecs::kDefaultPriority);

  registry_->AddSystem<engine::ecs::PositionComponent,
                       engine::ecs::VelocityComponent>(
      engine::ecs::MovementSystem::Update, engine::ecs::SystemType::Fixed,
      engine::ecs::kDefaultPriority);

  registry_->AddSystemClass(std::make_shared<systems::BoundarySystem>(),
                            engine::ecs::SystemType::Fixed,
                            engine::ecs::kDefaultPriority);

  registry_->AddSystemClass(
      std::make_shared<systems::CollisionSystem>(event_bus_, *script_engine_),
      engine::ecs::SystemType::Fixed, engine::ecs::kDefaultPriority);

  registry_->AddSystemClass(std::make_shared<systems::AnimationSystem>(),
                            engine::ecs::SystemType::Variable,
                            engine::ecs::kDefaultPriority);

  registry_->AddSystemClass(
      std::make_shared<systems::AISystem>(*script_engine_),
      engine::ecs::SystemType::Fixed, engine::ecs::kDefaultPriority);

  registry_->AddSystemClass(std::make_shared<systems::HealthSystem>(),
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
