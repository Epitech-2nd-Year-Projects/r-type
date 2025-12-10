#include "application.h"

#include <iomanip>
#include <memory>
#include <sstream>
#include <string>
#include <utility>

#include "engine/math/vector2.h"
#include "engine/render.h"
#include "engine/time/game_loop.h"
#include "input_sender.h"
#include "logging.h"

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

  LogLifecycle(engine::util::LogLevel::kInfo, "Engine runtime ready");
  LogLifecycle(engine::util::LogLevel::kDebug, "Entering main loop");
  LogConnectionStatus(engine::util::LogLevel::kInfo, config_.host, config_.port,
                      "connecting");

  const auto transport_error = transport_->Start(config_.host, config_.port);
  if (transport_error) {
    LogLifecycle(engine::util::LogLevel::kError,
                 std::string("Failed to start network transport: ") +
                     transport_error.message());
    return 1;
  }
  if (input_sender_) {
    input_sender_->Reset();
  }
  join_flow_.Begin(*transport_);

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
  const auto join_state = join_flow_.state();
  if (join_state == JoinState::kRefused) {
    LogLifecycle(engine::util::LogLevel::kError, join_flow_.status());
    return false;
  }
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
      // TODO: Dispatch world updates to gameplay systems.
    }
  }
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

  context.EndFrame();
  return true;
}

void Application::UpdateAudio(engine::time::TimeDelta dt,
                               JoinState join_state) {
  if (!audio_manager_) {
    return;
  }

  const bool connected = join_state == JoinState::kConnected;
  const bool was_connected = last_join_state_ == JoinState::kConnected;
  const bool transport_running =
      transport_ ? transport_->running() : false;

  if (connected && !music_blocked_) {
    music_allowed_ = true;
  }

  const bool lost_connection = (!connected && was_connected) || !transport_running;
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
