#include "application.h"

#include <cstdint>
#include <iomanip>
#include <memory>
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

namespace client {

namespace {

constexpr float kDisconnectFadeSeconds = 1.25f;
constexpr float kGameOverFadeSeconds = 1.5f;

}  // namespace

Application::Application(ClientConfig config)
    : config_(std::move(config)),
      transport_(std::make_shared<NetworkTransport>()),
      join_flow_(config_.player_name, config_.room_code) {}

int Application::Run() {
  ConfigureClientLogging(config_.log_level);
  LogLifecycle(engine::util::LogLevel::kInfo, "Starting R-Type client");
  LogConnectionStatus(engine::util::LogLevel::kInfo, config_.host, config_.port,
                      "target configured");

  engine::core::EngineRuntimeConfig runtime_config;
  runtime_config.window_config.title = "R-Type Client";
  runtime_config.window_config.size = engine::math::Vector2i(1280, 720);
  runtime_config.window_config.vsync = true;
  runtime_config.window_config.target_fps = 60;
  runtime_config.log_level = config_.log_level;

  engine_ = engine::core::EngineRuntime::Create(runtime_config);
  if (!engine_) {
    LogLifecycle(engine::util::LogLevel::kCritical,
                 "Failed to initialize engine runtime");
    return 1;
  }

  input_layer_ = std::make_unique<InputLayer>(engine_->Input());
  input_layer_->ApplyDefaultBindings();
  input_sender_ =
      std::make_unique<InputSender>(*input_layer_, world_update_receiver_);

  if (engine_->Audio()) {
    audio_manager_ = std::make_unique<AudioManager>(*engine_->Audio());
    audio_manager_->LoadAssets();
    LogLifecycle(engine::util::LogLevel::kInfo, "Audio manager initialized");
  }

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
  if (!StartConnection()) {
    return 1;
  }

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

bool Application::Tick(engine::time::TimeDelta dt) {
  if (!engine_->Pump()) {
    LogLifecycle(engine::util::LogLevel::kError,
                 "Engine pump stopped the client loop");
    return false;
  }

  if (input_layer_) {
    input_layer_->Update();
  }

  join_flow_.Update(*transport_);
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
    const bool connected = join_state == JoinState::kConnected;
    input_sender_->Update(dt, connected);
  }
  if (world_update_receiver_.running()) {
    WorldUpdateMessage message;
    while (world_update_receiver_.TryPop(message)) {
      if (message.type == protocol::message_type::MessageType::kPlayerDied) {
        HandleGameOverAudio();
      }
      if (message.type == protocol::message_type::MessageType::kServerCommand) {
        if (const auto command =
                std::get_if<protocol::CommandPayload>(&message.payload)) {
          HandleServerCommand(*command);
        }
      }
      // TODO: Dispatch world updates to gameplay systems.
      if (join_flow_.state() != JoinState::kConnected) {
        break;
      }
    }
  }
  join_state = join_flow_.state();
  MonitorConnection(join_state);
  join_state = join_flow_.state();
  HandleReconnectInput(join_state);
  UpdateAudio(dt, join_state);

  auto& window = engine_->Window();
  auto& context = window.GetRenderContext();
  auto& renderer = context.Get2DRenderer();

  const float fps = dt.as_seconds() > 0.0f ? 1.0f / dt.as_seconds() : 0.0f;

  std::ostringstream hud;
  hud << std::fixed << std::setprecision(1) << "FPS: " << fps;
  const auto connection_status = join_flow_.status();

  context.BeginFrame();
  context.Clear(engine::render::Color::FromBytes(12, 12, 16));

  renderer.DrawText("R-Type Client", {24.0f, 28.0f}, 28.0f,
                    engine::render::Color::White());
  renderer.DrawText(hud.str(), {24.0f, 64.0f}, 20.0f,
                    engine::render::Color::FromBytes(200, 200, 200));
  renderer.DrawText(connection_status, {24.0f, 96.0f}, 18.0f,
                    engine::render::Color::FromBytes(180, 220, 255));
  if (const auto player_id = join_flow_.player_id()) {
    renderer.DrawText("Player ID: " + std::to_string(*player_id),
                      {24.0f, 120.0f}, 18.0f,
                      engine::render::Color::FromBytes(180, 220, 255));
  }
  if (join_state != JoinState::kConnected &&
      join_state != JoinState::kConnecting) {
    renderer.DrawText("Press R to reconnect", {24.0f, 148.0f}, 18.0f,
                      engine::render::Color::FromBytes(255, 196, 128));
  }

  context.EndFrame();
  return true;
}

bool Application::StartConnection() {
  LogConnectionStatus(engine::util::LogLevel::kInfo, config_.host, config_.port,
                      "connecting");

  world_update_receiver_.Stop();
  if (transport_) {
    transport_->Stop();
  }
  if (input_sender_) {
    input_sender_->Reset();
  }

  const auto transport_error = transport_->Start(config_.host, config_.port);
  if (transport_error) {
    LogLifecycle(engine::util::LogLevel::kError,
                 std::string("Failed to start network transport: ") +
                     transport_error.message());
    join_flow_.MarkDisconnected("Transport failed to start");
    return false;
  }

  reconnect_requested_ = false;
  join_flow_.Begin(*transport_);
  return true;
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

  world_update_receiver_.Stop();
  if (transport_) {
    transport_->Stop();
  }
  if (input_sender_) {
    input_sender_->Reset();
  }
  join_flow_.MarkDisconnected(reason);
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

}  // namespace client
