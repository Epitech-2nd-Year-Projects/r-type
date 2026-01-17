#include "application.h"

#include <iostream>

#include "engine/app/engine_runtime.h"
#include "engine/ecs/registry.h"
#include "engine/input.h"
#include "engine/render/context.h"
#include "engine/render/renderer2d.h"
#include "engine/render/renderer3d.h"
#include "engine/render/window.h"
#include "engine/time/game_loop.h"
#include "logging.h"
#include "protocol/command.h"
#include "scene/fight_scene.h"

namespace rift::client {

Application::Application(RiftConfig config) : config_(std::move(config)) {}

Application::~Application() = default;

int Application::Run() {
  if (!Initialize()) {
    return 1;
  }

  if (config_.debug) {
    ConfigureRiftClientLogging(engine::util::LogLevel::kDebug);
  } else {
    ConfigureRiftClientLogging(engine::util::LogLevel::kInfo);
  }

  LogLifecycle(engine::util::LogLevel::kInfo, "Rift Client starting");

  if (auto error = network_->StartConnection()) {
    LogLifecycle(engine::util::LogLevel::kError, *error);
    std::cerr << "Connection failed: " << *error << std::endl;
    return 1;
  }

  LogLifecycle(engine::util::LogLevel::kInfo,
               "Connecting to " + config_.host + ":" +
                   std::to_string(config_.port));

  engine::time::VariableTimestepLoop loop(config_.target_fps);
  loop.run([this](engine::time::TimeDelta dt) { return Tick(dt); });

  LogLifecycle(engine::util::LogLevel::kInfo, "Rift Client shutting down");
  return 0;
}

bool Application::Initialize() {
  engine::app::EngineRuntimeConfig engine_config{};
  engine_config.window_config.title = "Rift";
  engine_config.window_config.size = {config_.resolution_width,
                                       config_.resolution_height};
  engine_config.window_config.fullscreen = config_.fullscreen;
  engine_config.window_config.vsync = config_.vsync;
  engine_config.window_config.target_fps = config_.target_fps;

  engine_ = engine::app::EngineRuntime::Create(engine_config);
  if (!engine_) {
    std::cerr << "Failed to initialize engine runtime" << std::endl;
    return false;
  }

  render_size_ = {config_.resolution_width, config_.resolution_height};

  network_ = std::make_unique<NetworkSession>(config_);

  input_layer_ = std::make_unique<FightInputLayer>(engine_->Input());
  input_layer_->ApplyDefaultBindings();

  input_sender_ = std::make_unique<InputSender>(*input_layer_,
                                                 network_->UpdateReceiver());

  scene_ = std::make_unique<FightScene>(*this);

  return true;
}

bool Application::Tick(engine::time::TimeDelta dt) {
  if (!engine_->Pump()) {
    return false;
  }

  if (should_quit_) {
    return false;
  }

  input_layer_->Update();

  auto events = network_->Update(dt);
  HandleNetworkEvents(events);

  if (events.stop_requested) {
    return false;
  }

  UpdateGameState();

  if (state_ == RiftClientState::kMatchOver) {
    UpdateMatchOverState(dt);
  }

  const bool sending_enabled =
      state_ == RiftClientState::kPlaying && IsConnected();
  input_sender_->Update(dt, sending_enabled);

  scene_->Update(dt);

  Render();

  return true;
}

void Application::HandleNetworkEvents(const NetworkEvents& events) {
  if (events.connected) {
    LogNetwork(engine::util::LogLevel::kInfo, "Connected to server");
    state_ = RiftClientState::kWaitingRoom;
  }

  if (events.connection_failed.has_value()) {
    LogNetwork(engine::util::LogLevel::kError, *events.connection_failed);
    should_quit_ = true;
  }

  if (events.disconnected.has_value()) {
    LogNetwork(engine::util::LogLevel::kWarn, *events.disconnected);
    should_quit_ = true;
  }

  if (events.match_started) {
    LogNetwork(engine::util::LogLevel::kInfo, "Match started!");
    state_ = RiftClientState::kPlaying;
  }

  if (events.match_over.has_value()) {
    LogNetwork(engine::util::LogLevel::kInfo, "Match over");
    state_ = RiftClientState::kMatchOver;
    match_over_timer_ = 0.0f;
  }
}

void Application::UpdateGameState() {
  if (state_ == RiftClientState::kWaitingRoom && !ready_sent_ && IsConnected()) {
    protocol::CommandPayload ready_cmd;
    ready_cmd.command_id =
        static_cast<std::uint16_t>(protocol::CommandType::kSetReady);
    if (network_->EnqueueCommand(ready_cmd)) {
      ready_sent_ = true;
      LogNetwork(engine::util::LogLevel::kInfo, "Sent ready command");
    }
  }
}

void Application::UpdateMatchOverState(engine::time::TimeDelta dt) {
  constexpr float kMatchOverDelaySeconds = 3.0f;
  match_over_timer_ += dt.as_seconds();
  if (match_over_timer_ >= kMatchOverDelaySeconds) {
    should_quit_ = true;
  }
}

void Application::Render() {
  auto& context = engine_->RenderContext();
  auto& renderer = engine_->Renderer();

  context.BeginFrame();
  context.Clear(engine::render::Color(20, 20, 30, 255));

  scene_->DrawBackground(renderer);
  scene_->Draw(renderer);

  context.EndFrame();
}

engine::render::Renderer2D& Application::Renderer() {
  return engine_->Renderer();
}

engine::render::Renderer3D& Application::Renderer3D() {
  return engine_->RenderContext().Get3DRenderer();
}

engine::input::InputManager& Application::Input() { return engine_->Input(); }

engine::render::Window& Application::Window() { return engine_->Window(); }

engine::math::Vector2i Application::RenderSize() const { return render_size_; }

engine::ecs::Registry& Application::World() { return network_->World(); }

const engine::ecs::Registry& Application::World() const {
  return network_->World();
}

bool Application::EnqueueCommand(const protocol::CommandPayload& payload) {
  return network_->EnqueueCommand(payload);
}

std::optional<std::uint32_t> Application::LocalPlayerId() const {
  return network_->LocalPlayerId();
}

std::optional<float> Application::LatestLatencyMs() const {
  return network_->LatestLatencyMs();
}

RiftClientState Application::State() const { return state_; }

bool Application::IsConnected() const {
  return network_->join_state() == JoinState::kConnected;
}

FightActionState Application::GetInputState() const {
  return input_layer_ ? input_layer_->state() : FightActionState{};
}

std::uint32_t Application::RoundTimerMs() const {
  return network_->RoundTimerMs();
}

}  // namespace rift::client
