#include "application.h"

#include <string>
#include <string_view>
#include <utility>

#include "audio_controller.h"
#include "client_asset_manager.h"
#include "client_runtime.h"
#include "constants/client_constants.h"
#include "constants/config_keys.h"
#include "constants/ui_constants.h"
#include "engine/time/game_loop.h"
#include "input/input_coordinator.h"
#include "logging.h"
#include "network_session.h"
#include "scene_manager.h"
#include "ui/menu_background.h"

namespace client {
namespace {

std::string_view ToString(ClientState state) {
  switch (state) {
    case ClientState::kMainMenu:
      return "MainMenu";
    case ClientState::kLobby:
      return "Lobby";
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
      runtime_(std::make_unique<ClientRuntime>()),
      scene_manager_(std::make_unique<SceneManager>(*this)),
      audio_(std::make_unique<AudioController>()),
      network_(std::make_unique<NetworkSession>(config_)),
      input_(std::make_unique<InputCoordinator>()) {}

Application::~Application() = default;

int Application::Run() {
  ConfigureClientLogging(config_.log_level);
  LogLifecycle(engine::util::LogLevel::kInfo, "Starting R-Type client");
  LogConnectionStatus(engine::util::LogLevel::kInfo, config_.host, config_.port,
                      "target configured");

  if (!runtime_->Initialize(config_)) {
    return 1;
  }

  runtime_->AttachWorld(network_->World());
  input_->Initialize(runtime_->Input(), network_->UpdateReceiver());
  auto audio_engine = runtime_->Audio();
  if (audio_engine) {
    audio_->Initialize(*audio_engine);
  }

  assets_ = std::make_unique<ClientAssetManager>(runtime_->Renderer());
  assets_->SetAudioEngine(audio_engine);
  assets_->PreloadMenuAssets();
  menu_background_ = std::make_unique<ui::MenuBackground>(
      constants::ui::MainMenu::kBackgroundVideoPath);

  UpdateRuntimeConfig();
  scene_manager_->Initialize(ClientState::kMainMenu);

  engine::time::VariableTimestepLoop loop(constants::client::kTargetFps);
  loop.run([this](engine::time::TimeDelta dt) { return Tick(dt); });

  audio_->StopMusic();
  network_->Shutdown();
  LogLifecycle(engine::util::LogLevel::kInfo, "Client shutdown complete");
  return 0;
}

engine::render::Renderer2D& Application::Renderer() {
  return runtime_->Renderer();
}

engine::input::InputManager& Application::Input() { return runtime_->Input(); }

const KeyBindings& Application::KeyBindingSet() const {
  return input_->key_bindings();
}

const KeyBindingService& Application::KeyBindingServiceRef() const {
  return input_->key_binding_service();
}

KeyBindingUpdateResult Application::UpdateKeyBinding(GameAction action,
                                                     engine::input::Key key) {
  return input_->UpdateKeyBinding(action, key);
}

engine::render::Window& Application::Window() { return runtime_->Window(); }

std::shared_ptr<engine::audio::AudioEngine> Application::Audio() {
  return runtime_->Audio();
}

ClientAssetManager& Application::Assets() { return *assets_; }

ui::MenuBackground& Application::MenuBackground() { return *menu_background_; }

engine::util::Configuration& Application::Config() {
  return runtime_->Config();
}

void Application::OnPlay() { scene_manager_->OnPlay(); }

void Application::OnOpenSettings() { scene_manager_->OnOpenSettings(); }

void Application::OnCloseSettings() { scene_manager_->OnCloseSettings(); }

void Application::OnQuitApplication() {
  StopNetworkSession();
  runtime_->RequestClose();
}

void Application::OnQuitToMenu() {
  StopNetworkSession();
  network_->Reset();
  input_->ResetReconnect();
  scene_manager_->OnQuitToMenu();
}

void Application::OnGamePause() { scene_manager_->OnGamePause(); }

void Application::OnGameResume() { scene_manager_->OnGameResume(); }

void Application::SetConnectionConfig(std::string host, int port,
                                      std::string player_name,
                                      std::string room_code,
                                      std::string room_password) {
  config_.host = std::move(host);
  config_.port = static_cast<std::uint16_t>(port);
  config_.player_name = std::move(player_name);
  config_.room_code = std::move(room_code);

  network_->SetConnectionConfig(config_.host, config_.port, config_.player_name,
                                config_.room_code, std::move(room_password));
  UpdateRuntimeConfig();
}

bool Application::StartConnection() {
  if (!scene_manager_->CanTransitionTo(ClientState::kConnecting)) {
    LogLifecycle(engine::util::LogLevel::kWarn,
                 "Ignoring connection attempt in state " +
                     std::string(ToString(scene_manager_->state())));
    return false;
  }

  LogConnectionStatus(engine::util::LogLevel::kInfo, config_.host, config_.port,
                      "connecting");

  StopNetworkSession();

  if (const auto error = network_->StartConnection()) {
    LogLifecycle(engine::util::LogLevel::kError, *error);
    scene_manager_->OnDisconnect(*error);
    return false;
  }

  input_->ResetReconnect();
  scene_manager_->TransitionTo(ClientState::kConnecting);
  return true;
}

void Application::RefreshRoomList(std::string host, std::uint16_t port) {
  network_->RefreshRoomList(std::move(host), port);
}

void Application::CreateRoom(std::string host, std::uint16_t port,
                             const std::string& room_name, bool is_private,
                             std::string room_password,
                             std::uint16_t max_players) {
  network_->CreateRoom(std::move(host), port, room_name, is_private,
                       std::move(room_password), max_players);
}

const std::vector<protocol::RoomSummary>& Application::RoomDirectoryRooms()
    const {
  return network_->RoomDirectoryRooms();
}

std::string Application::RoomDirectoryStatus() const {
  return network_->RoomDirectoryStatus();
}

std::optional<protocol::CreateRoomResponsePayload>
Application::ConsumeLastRoomCreation() {
  return network_->ConsumeLastRoomCreation();
}

engine::ecs::Registry& Application::World() { return network_->World(); }

const engine::ecs::Registry& Application::World() const {
  return network_->World();
}

bool Application::EnqueueCommand(const protocol::CommandPayload& payload) {
  return network_->EnqueueCommand(payload);
}

std::optional<std::uint32_t> Application::CurrentWave() const {
  return network_->CurrentWave();
}

std::optional<float> Application::LatestLatencyMs() const {
  return network_->LatestLatencyMs();
}

std::optional<std::uint32_t> Application::LocalPlayerId() const {
  return network_->LocalPlayerId();
}

std::string_view Application::ConnectionStatus() const {
  return network_->JoinStatus();
}

bool Application::ConnectionActive() const {
  return network_->join_state() == JoinState::kConnected &&
         network_->TransportRunning();
}

bool Application::Tick(engine::time::TimeDelta dt) {
  if (!runtime_->Pump()) {
    LogLifecycle(engine::util::LogLevel::kError,
                 "Engine pump stopped the client loop");
    return false;
  }

  const auto events = network_->Update(dt, *audio_);
  if (events.stop_requested) {
    return false;
  }
  HandleNetworkEvents(events);

  scene_manager_->Update(dt);

  const bool input_captured = scene_manager_->IsInputCaptured();
  runtime_->Input().SetActionsEnabled(!input_captured);

  input_->Update(dt, network_->join_state(), scene_manager_->state());
  if (input_->ShouldReconnect(network_->join_state(), input_captured)) {
    StartConnection();
  }

  const bool connected = network_->join_state() == JoinState::kConnected;
  audio_->Update(dt, scene_manager_->state(),
                 scene_manager_->settings_return_state(), connected,
                 network_->TransportRunning());

  runtime_->RenderFrame(dt, scene_manager_->state(),
                        scene_manager_->CurrentScene(), network_->World(),
                        network_->LatestLatencyMs());

  scene_manager_->CommitSceneChange();
  return true;
}

void Application::StopNetworkSession() {
  network_->Stop();
  runtime_->ResetWorld();
  audio_->Reset();
  input_->ResetSender();
}

void Application::HandleNetworkEvents(const NetworkEvents& events) {
  const bool should_stop =
      events.connection_failed.has_value() || events.disconnected.has_value();
  if (should_stop) {
    StopNetworkSession();
  }
  if (events.connection_failed.has_value()) {
    scene_manager_->OnConnectionFailed(*events.connection_failed);
  }
  if (events.disconnected.has_value()) {
    scene_manager_->OnDisconnect(*events.disconnected);
  }
  if (events.connected) {
    scene_manager_->OnConnected();
  }
  if (events.game_over.has_value()) {
    scene_manager_->OnGameOver(*events.game_over);
  }
}

void Application::UpdateRuntimeConfig() {
  auto& runtime_config_store = runtime_->Config();
  runtime_config_store.Set(std::string(constants::config::kClientHost),
                           config_.host);
  runtime_config_store.Set(std::string(constants::config::kClientPort),
                           std::to_string(config_.port));
  runtime_config_store.Set(std::string(constants::config::kClientDebug),
                           config_.debug ? "true" : "false");
  runtime_config_store.Set(
      std::string(constants::config::kClientLogLevel),
      std::string(engine::util::ToString(config_.log_level)));
  runtime_config_store.Set(std::string(constants::config::kClientPlayerName),
                           config_.player_name);
  runtime_config_store.Set(std::string(constants::config::kClientRoomCode),
                           config_.room_code);
  runtime_config_store.Set(std::string(constants::config::kClientTimeoutMs),
                           std::to_string(config_.timeout_ms));
  runtime_config_store.Set(
      std::string(constants::config::kClientPingIntervalMs),
      std::to_string(config_.ping_interval_ms));
  runtime_config_store.Set(std::string(constants::config::kClientQueueSize),
                           std::to_string(config_.network_queue_size));
  runtime_config_store.Set(std::string(constants::config::kClientJoinRetryMs),
                           std::to_string(config_.join_retry_delay_ms));
  runtime_config_store.Set(
      std::string(constants::config::kClientJoinMaxAttempts),
      std::to_string(config_.join_max_attempts));
  runtime_config_store.Set(std::string(constants::config::kClientLobbyRetryMs),
                           std::to_string(config_.lobby_retry_delay_ms));
  runtime_config_store.Set(
      std::string(constants::config::kClientLobbyMaxAttempts),
      std::to_string(config_.lobby_max_attempts));

  if (!SaveClientConfig(config_)) {
    LogLifecycle(engine::util::LogLevel::kWarn,
                 "Failed to persist client config");
  }
}

}  // namespace client
