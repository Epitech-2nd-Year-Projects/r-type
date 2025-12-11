#include "application.h"

#include <cstdint>
#include <iomanip>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <utility>

#include "engine/math/vector2.h"
#include "engine/render.h"
#include "engine/time/game_loop.h"
#include "engine/time/monotonic_time.h"
#include "input_sender.h"
#include "logging.h"
#include "protocol/command.h"
#include "scene/connecting_scene.h"
#include "scene/disconnected_scene.h"
#include "scene/game_over_scene.h"
#include "scene/in_game_scene.h"
#include "scene/main_menu_scene.h"
#include "scene/pause_scene.h"
#include "scene/settings_scene.h"

namespace client {

namespace {

constexpr float kDisconnectFadeSeconds = 1.25f;
constexpr float kGameOverFadeSeconds = 1.5f;
const engine::math::Vector2i kBaseResolution{1600, 900};
constexpr engine::render::Color kClearColor =
    engine::render::Color::FromBytes(12, 12, 16);

const char* ToString(ClientState state) {
  switch (state) {
    case ClientState::kMainMenu:
      return "MainMenu";
    case ClientState::kSettings:
      return "Settings";
    case ClientState::kConnecting:
      return "Connecting";
    case ClientState::kInGame:
      return "InGame";
    case ClientState::kPaused:
      return "Paused";
    case ClientState::kGameOver:
      return "GameOver";
    case ClientState::kDisconnected:
      return "Disconnected";
  }
  return "Unknown";
}

}  // namespace

Application::Application(ClientConfig config)
    : config_(std::move(config)),
      transport_(std::make_shared<NetworkTransport>()),
      join_flow_(config_.player_name, config_.room_code),
      world_registry_(std::make_unique<engine::ecs::Registry>()),
      world_state_system_(
          std::make_unique<ecs::WorldStateSystem>(*world_registry_)),
      interpolation_system_(
          std::make_unique<ecs::InterpolationSystem>(*world_registry_)) {
  local_prediction_ =
      std::make_unique<LocalPrediction>(*world_registry_, join_flow_);
}

int Application::Run() {
  ConfigureClientLogging(config_.log_level);
  LogLifecycle(engine::util::LogLevel::kInfo, "Starting R-Type client");
  LogConnectionStatus(engine::util::LogLevel::kInfo, config_.host, config_.port,
                      "target configured");

  engine::core::EngineRuntimeConfig runtime_config;
  runtime_config.window_config.title = "R-Type Client";
  runtime_config.window_config.size = kBaseResolution;
  runtime_config.window_config.resizable = false;
  runtime_config.window_config.vsync = true;
  runtime_config.window_config.target_fps = 60;
  runtime_config.log_level = config_.log_level;
  runtime_config.window_backend_factory = engine::render::CreateRaylibBackend;

  const float aspect_ratio =
      static_cast<float>(runtime_config.window_config.size.x) /
      static_cast<float>(runtime_config.window_config.size.y);
  std::ostringstream window_info;
  window_info << "Window " << runtime_config.window_config.size.x << 'x'
              << runtime_config.window_config.size.y << " (" << std::fixed
              << std::setprecision(2) << aspect_ratio << ":1) raylib backend";
  LogLifecycle(engine::util::LogLevel::kInfo, window_info.str());

  engine_ = engine::core::EngineRuntime::Create(runtime_config);
  if (!engine_) {
    LogLifecycle(engine::util::LogLevel::kCritical,
                 "Failed to initialize engine runtime");
    return 1;
  }

  input_layer_ = std::make_unique<InputLayer>(engine_->Input());
  LoadKeyBindings();
  input_sender_ =
      std::make_unique<InputSender>(*input_layer_, world_update_receiver_);

  if (engine_->Audio()) {
    audio_manager_ = std::make_unique<AudioManager>(*engine_->Audio());
    audio_manager_->LoadAssets();
    LogLifecycle(engine::util::LogLevel::kInfo, "Audio manager initialized");
  }

  auto& input = engine_->Input();
  input.BindKey("Confirm", engine::input::Key::kEnter);
  input.BindKey("Cancel", engine::input::Key::kEscape);
  input.BindKey("Quit", engine::input::Key::kQ);

  auto& runtime_config_store = engine_->Config();
  runtime_config_store.Set("client.host", config_.host);
  runtime_config_store.Set("client.port", std::to_string(config_.port));
  runtime_config_store.Set("client.debug", config_.debug ? "true" : "false");
  runtime_config_store.Set(
      "client.log_level",
      std::string(engine::util::ToString(config_.log_level)));
  runtime_config_store.Set("client.player_name", config_.player_name);
  runtime_config_store.Set("client.room_code", config_.room_code);
  runtime_config_store.Set("client.timeout_ms",
                           std::to_string(config_.timeout_ms));

  LogLifecycle(engine::util::LogLevel::kInfo, "Engine runtime ready");
  LogLifecycle(engine::util::LogLevel::kDebug, "Entering main loop");
  ApplyState(ClientState::kMainMenu);

  engine::time::VariableTimestepLoop loop(
      static_cast<float>(runtime_config.window_config.target_fps));

  loop.run([this](engine::time::TimeDelta dt) { return Tick(dt); });
  if (audio_manager_) {
    audio_manager_->StopMusic();
  }
  world_update_receiver_.Stop();
  transport_->Stop();
  LogLifecycle(engine::util::LogLevel::kInfo, "Client shutdown complete");
  return 0;
}

void Application::SwitchScene(std::unique_ptr<Scene> scene) {
  current_scene_ = std::move(scene);
}

void Application::OnConnected() { TransitionTo(ClientState::kInGame); }

void Application::OnConnectionFailed(const std::string& reason) {
  StopNetworkSession();
  TransitionTo(ClientState::kDisconnected, reason);
}

void Application::OnGameStart() { TransitionTo(ClientState::kInGame); }

void Application::OnGamePause() { TransitionTo(ClientState::kPaused); }

void Application::OnGameResume() { TransitionTo(ClientState::kInGame); }

void Application::OnGameOver() {
  if (state_ == ClientState::kGameOver) {
    return;
  }
  TransitionTo(ClientState::kGameOver);
}

void Application::OnDisconnect(std::string reason) {
  StopNetworkSession();
  TransitionTo(ClientState::kDisconnected, std::move(reason));
}

void Application::OnQuitToMenu() {
  StopNetworkSession();
  join_flow_.Reset();
  reconnect_requested_ = false;
  TransitionTo(ClientState::kMainMenu);
}

void Application::OnOpenSettings() { TransitionTo(ClientState::kSettings); }

bool Application::Tick(engine::time::TimeDelta dt) {
  if (!engine_->Pump()) {
    LogLifecycle(engine::util::LogLevel::kError,
                 "Engine pump stopped the client loop");
    return false;
  }

  if (input_layer_) {
    input_layer_->Update();
  }

  if (transport_) {
    join_flow_.Update(*transport_);
  }
  ProcessJoinState(join_flow_.state());
  if (current_scene_) {
    current_scene_->Update(dt);
  }

  auto join_state = join_flow_.state();
  if (join_state == JoinState::kConnected &&
      !world_update_receiver_.running()) {
    if (!world_update_receiver_.Start(transport_)) {
      LogLifecycle(engine::util::LogLevel::kError,
                   "Failed to start world update receiver");
      return false;
    }
  }
  if (input_sender_) {
    const bool accepts_input =
        join_state == JoinState::kConnected && state_ == ClientState::kInGame;
    input_sender_->Update(dt, accepts_input);
  }
  if (world_update_receiver_.running()) {
    WorldUpdateMessage message;
    while (world_update_receiver_.TryPop(message)) {
      if (message.type == protocol::message_type::MessageType::kWorldSnapshot) {
        if (const auto snapshot =
                std::get_if<protocol::WorldSnapshotPayload>(&message.payload)) {
          std::optional<engine::math::Vector2f> predicted_before;
          if (local_prediction_) {
            predicted_before = local_prediction_->CapturePredictedPosition();
          }
          const std::uint64_t receipt_ms = engine::time::NowMilliseconds();
          world_state_system_->ApplySnapshot(*snapshot, receipt_ms);
          if (local_prediction_) {
            local_prediction_->OnSnapshotApplied(predicted_before);
          }
        }
      }
      if (message.type == protocol::message_type::MessageType::kPlayerDied) {
        HandleGameOverAudio();
        if (state_ == ClientState::kInGame || state_ == ClientState::kPaused) {
          OnGameOver();
        }
      }
      if (message.type == protocol::message_type::MessageType::kServerCommand) {
        if (const auto command =
                std::get_if<protocol::CommandPayload>(&message.payload)) {
          HandleServerCommand(*command);
        }
      }
      if (join_flow_.state() != JoinState::kConnected) {
        break;
      }
    }
  }
  join_state = join_flow_.state();
  MonitorConnection(join_state);
  join_state = join_flow_.state();
  HandleReconnectInput(join_state);
  const bool should_predict = local_prediction_ && input_layer_ &&
                              join_state == JoinState::kConnected &&
                              state_ == ClientState::kInGame;
  if (should_predict) {
    local_prediction_->Update(dt, input_layer_->state());
  }
  if (interpolation_system_) {
    interpolation_system_->Update(dt);
  }
  UpdateAudio(dt, join_state);

  auto& context = engine_->RenderContext();
  auto& renderer = engine_->Renderer();

  const float fps = dt.as_seconds() > 0.0f ? 1.0f / dt.as_seconds() : 0.0f;

  std::ostringstream hud;
  hud << std::fixed << std::setprecision(1) << "FPS: " << fps;

  context.BeginFrame();
  context.Clear(kClearColor);

  if (current_scene_) {
    current_scene_->Draw(renderer);
  }

  renderer.DrawText(hud.str(), {24.0f, 24.0f}, 20.0f,
                    engine::render::Color::FromBytes(200, 200, 200));

  context.EndFrame();
  return true;
}

bool Application::StartConnection() {
  if (!IsTransitionAllowed(ClientState::kConnecting)) {
    LogLifecycle(engine::util::LogLevel::kWarn,
                 "Ignoring connection attempt in state " +
                     std::string(ToString(state_)));
    return false;
  }

  LogConnectionStatus(engine::util::LogLevel::kInfo, config_.host, config_.port,
                      "connecting");

  StopNetworkSession();

  const auto transport_error = transport_->Start(config_.host, config_.port);
  if (transport_error) {
    const std::string reason =
        std::string("Failed to start network transport: ") +
        transport_error.message();
    LogLifecycle(engine::util::LogLevel::kError, reason);
    join_flow_.MarkDisconnected(reason);
    OnDisconnect(reason);
    return false;
  }

  reconnect_requested_ = false;
  join_flow_.Begin(*transport_);
  ApplyState(ClientState::kConnecting);
  return true;
}

void Application::SetConnectionConfig(std::string host, int port,
                                      std::string player_name) {
  config_.host = std::move(host);
  config_.port = port;
  config_.player_name = std::move(player_name);

  auto& runtime_config_store = engine_->Config();
  runtime_config_store.Set("client.host", config_.host);
  runtime_config_store.Set("client.port", std::to_string(config_.port));
  runtime_config_store.Set("client.player_name", config_.player_name);

  join_flow_ = JoinFlow(config_.player_name, config_.room_code);
}

void Application::HandleServerCommand(const protocol::CommandPayload& payload) {
  if (payload.command_id ==
      static_cast<std::uint16_t>(protocol::CommandType::kDisconnectNotice)) {
    const std::string reason =
        payload.payload.empty() ? "Disconnected by server" : payload.payload;
    LogLifecycle(engine::util::LogLevel::kWarn, reason);
    HandleConnectionLost(reason);
  }
}

void Application::MonitorConnection(JoinState join_state) {
  if (join_state != JoinState::kConnected) {
    return;
  }

  if (!transport_ || !transport_->running()) {
    HandleConnectionLost("Connection closed");
    return;
  }

  const auto last_receive = transport_->last_receive_ms();
  if (last_receive == 0) {
    return;
  }

  const auto now_ms = engine::time::NowMilliseconds();
  const auto silence_ms = now_ms >= last_receive ? now_ms - last_receive : 0;
  if (silence_ms > config_.timeout_ms) {
    HandleConnectionLost("Timed out waiting for server");
  }
}

void Application::HandleConnectionLost(std::string_view reason) {
  if (join_flow_.state() == JoinState::kDisconnected) {
    return;
  }

  StopNetworkSession();
  join_flow_.MarkDisconnected(reason);
  OnDisconnect(std::string(reason));
}

bool Application::UpdateKeyBinding(GameAction action, engine::input::Key key) {
  key_bindings_.Set(action, key);
  if (input_layer_) {
    input_layer_->ApplyBindings(key_bindings_);
  }
  return SaveKeyBindings();
}

void Application::LoadKeyBindings() {
  KeyBindings bindings = KeyBindings::Default();
  const bool loaded = bindings.LoadFromFile(keybindings_path_);
  if (!loaded) {
    LogLifecycle(engine::util::LogLevel::kDebug,
                 "Key bindings config not found, applying defaults");
  }
  key_bindings_ = std::move(bindings);
  if (input_layer_) {
    input_layer_->ApplyBindings(key_bindings_);
  }
}

bool Application::SaveKeyBindings() {
  if (!key_bindings_.SaveToFile(keybindings_path_)) {
    LogLifecycle(engine::util::LogLevel::kWarn,
                 "Failed to persist key bindings to " +
                     keybindings_path_.string());
    return false;
  }
  return true;
}

void Application::HandleReconnectInput(JoinState join_state) {
  const bool request =
      input_layer_ ? input_layer_->ConsumeReconnectRequest() : false;
  if (request) {
    reconnect_requested_ = true;
  }

  const bool can_retry = join_state != JoinState::kConnected &&
                         join_state != JoinState::kConnecting;
  if (reconnect_requested_ && can_retry) {
    reconnect_requested_ = false;
    StartConnection();
  }
}

void Application::UpdateAudio(engine::time::TimeDelta dt,
                              JoinState join_state) {
  if (!audio_manager_) {
    return;
  }

  const bool connected = join_state == JoinState::kConnected;
  const bool was_connected = last_join_state_ == JoinState::kConnected;
  const bool transport_running = transport_ ? transport_->running() : false;

  if (connected && !music_blocked_) {
    music_allowed_ = true;
  }

  const bool lost_connection =
      (!connected && was_connected) || !transport_running;
  if (lost_connection) {
    music_allowed_ = false;
    music_blocked_ = false;
    if (audio_manager_->MusicActive()) {
      audio_manager_->FadeOutMusic(kDisconnectFadeSeconds);
    }
  }

  if (music_allowed_ && !music_blocked_ && !audio_manager_->MusicActive()) {
    audio_manager_->PlayMusic(MusicType::kBackground);
  }

  audio_manager_->Update(dt.as_seconds());
  last_join_state_ = join_state;
}

void Application::HandleGameOverAudio() {
  if (!audio_manager_) {
    return;
  }
  music_allowed_ = false;
  music_blocked_ = true;
  audio_manager_->FadeOutMusic(kGameOverFadeSeconds);
}

void Application::ProcessJoinState(JoinState join_state) {
  if (state_ == ClientState::kConnecting &&
      join_state == JoinState::kConnected) {
    OnConnected();
    return;
  }

  if (state_ == ClientState::kConnecting && join_state == JoinState::kRefused) {
    OnConnectionFailed(join_flow_.status());
  }
}

void Application::ApplyState(ClientState next_state, std::string reason) {
  if (next_state == ClientState::kDisconnected) {
    disconnect_reason_ =
        reason.empty() ? "Disconnected from server" : std::move(reason);
  } else {
    disconnect_reason_.clear();
  }

  const bool scene_missing = !current_scene_;
  const bool state_changed = state_ != next_state;
  state_ = next_state;

  const bool refresh_scene =
      scene_missing || state_changed || state_ == ClientState::kDisconnected;
  if (!refresh_scene) {
    return;
  }

  switch (state_) {
    case ClientState::kMainMenu:
      SwitchScene(std::make_unique<MainMenuScene>(*this));
      break;
    case ClientState::kSettings:
      SwitchScene(std::make_unique<SettingsScene>(*this));
      break;
    case ClientState::kConnecting:
      SwitchScene(std::make_unique<ConnectingScene>(*this));
      break;
    case ClientState::kInGame:
      SwitchScene(std::make_unique<InGameScene>(*this));
      break;
    case ClientState::kPaused:
      SwitchScene(std::make_unique<PauseScene>(*this));
      break;
    case ClientState::kGameOver:
      SwitchScene(std::make_unique<GameOverScene>(*this));
      break;
    case ClientState::kDisconnected:
      SwitchScene(
          std::make_unique<DisconnectedScene>(*this, disconnect_reason_));
      break;
  }
}

bool Application::TransitionTo(ClientState next_state, std::string reason) {
  if (!IsTransitionAllowed(next_state)) {
    LogLifecycle(engine::util::LogLevel::kWarn,
                 "Rejected state transition " + std::string(ToString(state_)) +
                     " -> " + std::string(ToString(next_state)));
    return false;
  }

  ApplyState(next_state, std::move(reason));
  return true;
}

bool Application::IsTransitionAllowed(ClientState next_state) const {
  switch (state_) {
    case ClientState::kMainMenu:
      return next_state == ClientState::kConnecting ||
             next_state == ClientState::kSettings ||
             next_state == ClientState::kMainMenu ||
             next_state == ClientState::kDisconnected;
    case ClientState::kSettings:
      return next_state == ClientState::kMainMenu;
    case ClientState::kConnecting:
      return next_state == ClientState::kInGame ||
             next_state == ClientState::kDisconnected ||
             next_state == ClientState::kMainMenu;
    case ClientState::kInGame:
      return next_state == ClientState::kPaused ||
             next_state == ClientState::kGameOver ||
             next_state == ClientState::kDisconnected;
    case ClientState::kPaused:
      return next_state == ClientState::kInGame ||
             next_state == ClientState::kGameOver ||
             next_state == ClientState::kMainMenu ||
             next_state == ClientState::kDisconnected;
    case ClientState::kGameOver:
      return next_state == ClientState::kMainMenu ||
             next_state == ClientState::kDisconnected;
    case ClientState::kDisconnected:
      return next_state == ClientState::kConnecting ||
             next_state == ClientState::kMainMenu;
  }
  return false;
}

void Application::StopNetworkSession() {
  world_update_receiver_.Stop();
  if (transport_) {
    transport_->Stop();
  }
  if (world_state_system_) {
    world_state_system_->Reset();
  }
  if (input_sender_) {
    input_sender_->Reset();
  }
  if (local_prediction_) {
    local_prediction_->Reset();
  }
}

}  // namespace client
