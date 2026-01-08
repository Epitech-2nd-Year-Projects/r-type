#include "engine/game_runtime.h"

#include <chrono>
#include <thread>
#include <variant>

#include "engine/audio/raylib_audio_engine.h"
#include "engine/net/events.h"
#include "engine/util/logging.h"

namespace engine {

GameRuntime::GameRuntime()
    : config_(Config{}),
      registry_(std::make_unique<ecs::Registry>()),
      event_bus_(std::make_unique<event::EventBus>()),
      snapshot_buffer_(std::make_unique<render::SnapshotBuffer>()),
      frame_interpolator_(
          std::make_unique<render::FrameInterpolator>(*snapshot_buffer_)),
      socket_(std::make_unique<net::UdpSocket>()),
      console_(std::make_unique<console::Console>()),
      console_overlay_(std::make_unique<console::ConsoleOverlay>(*console_)),
      frame_profiler_(std::make_unique<profiling::FrameProfiler>()),
      profiling_overlay_(std::make_unique<profiling::ProfilingOverlay>()),
      network_debugger_(std::make_unique<debug::NetworkDebugger>()),
      component_inspector_registry_(
          std::make_unique<debug::ComponentInspectorRegistry>()),
      debug_suite_(std::make_unique<debug::DebugSuite>(
          *registry_, *component_inspector_registry_)) {
  profiling_overlay_->SetFrameProfiler(*frame_profiler_);
  debug_suite_->SetConsoleOverlay(*console_overlay_);
  debug_suite_->SetProfilingOverlay(*profiling_overlay_);
  debug_suite_->SetNetworkDebugger(*network_debugger_);
}

GameRuntime::GameRuntime(Config config)
    : config_(config),
      registry_(std::make_unique<ecs::Registry>()),
      event_bus_(std::make_unique<event::EventBus>()),
      snapshot_buffer_(std::make_unique<render::SnapshotBuffer>()),
      frame_interpolator_(
          std::make_unique<render::FrameInterpolator>(*snapshot_buffer_)),
      socket_(std::make_unique<net::UdpSocket>()),
      audio_dispatcher_(
          std::make_unique<audio::AudioDispatcher>(audio_command_queue_)),
      console_(std::make_unique<console::Console>()),
      console_overlay_(std::make_unique<console::ConsoleOverlay>(*console_)),
      frame_profiler_(std::make_unique<profiling::FrameProfiler>()),
      profiling_overlay_(std::make_unique<profiling::ProfilingOverlay>()),
      network_debugger_(std::make_unique<debug::NetworkDebugger>()),
      component_inspector_registry_(
          std::make_unique<debug::ComponentInspectorRegistry>()),
      debug_suite_(std::make_unique<debug::DebugSuite>(
          *registry_, *component_inspector_registry_)) {
  profiling_overlay_->SetFrameProfiler(*frame_profiler_);
  debug_suite_->SetConsoleOverlay(*console_overlay_);
  debug_suite_->SetProfilingOverlay(*profiling_overlay_);
  debug_suite_->SetNetworkDebugger(*network_debugger_);
}

GameRuntime::~GameRuntime() { Stop(); }

void GameRuntime::Start() {
  if (running_.exchange(true)) {
    return;
  }

  logic_thread_ =
      std::make_unique<std::thread>(&GameRuntime::LogicThreadMain, this);
  network_thread_ =
      std::make_unique<std::thread>(&GameRuntime::NetworkThreadMain, this);
  audio_thread_ =
      std::make_unique<std::thread>(&GameRuntime::AudioThreadMain, this);
  debug_thread_ =
      std::make_unique<std::thread>(&GameRuntime::DebugThreadMain, this);
}

void GameRuntime::Stop() {
  bool expected = true;
  if (!running_.compare_exchange_strong(expected, false)) {
    return;
  }

  if (logic_thread_ && logic_thread_->joinable()) logic_thread_->join();
  if (network_thread_ && network_thread_->joinable()) network_thread_->join();
  if (audio_thread_ && audio_thread_->joinable()) audio_thread_->join();
  if (debug_thread_ && debug_thread_->joinable()) debug_thread_->join();

  logic_thread_.reset();
  network_thread_.reset();
  audio_thread_.reset();
  debug_thread_.reset();
}

bool GameRuntime::Running() const { return running_.load(); }

void GameRuntime::StartNetwork(std::uint16_t port) {
  if (socket_) {
    socket_->open(net::UdpSocket::Protocol::kIpv4);
    socket_->bind(net::Endpoint::AnyIpv4(port));
  }
}

std::uint16_t GameRuntime::GetBoundPort() const {
  if (socket_ && socket_->is_open()) {
    return socket_->local_endpoint().port();
  }
  return 0;
}

ecs::Registry& GameRuntime::Registry() { return *registry_; }

event::EventBus& GameRuntime::EventBus() { return *event_bus_; }

audio::AudioDispatcher& GameRuntime::GetAudioDispatcher() {
  return *audio_dispatcher_;
}

void GameRuntime::RunMainThread(
    std::function<bool(time::TimeDelta)> render_callback) {
  auto last_time = std::chrono::steady_clock::now();

  while (running_.load()) {
    auto current_time = std::chrono::steady_clock::now();
    auto delta_time = time::TimeDelta::from_seconds(
        std::chrono::duration<float>(current_time - last_time).count());
    last_time = current_time;

    event_bus_->FlushChannel(util::ThreadId::kMain);

    std::uint64_t now_ns =
        std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::steady_clock::now().time_since_epoch())
            .count();

    frame_profiler_->RecordFrame(delta_time);
    debug_suite_->Draw();

    [[maybe_unused]] auto sprites = frame_interpolator_->Interpolate(now_ns);

    if (render_callback) {
      if (!render_callback(delta_time)) {
        Stop();
      }
    } else {
      std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }
  }
}

void GameRuntime::LogicThreadMain() {
  using clock = std::chrono::steady_clock;
  const auto timestep = config_.logic_timestep;
  auto previous = clock::now();
  time::TimeDelta accumulator = time::TimeDelta::zero();

  while (running_.load(std::memory_order_acquire)) {
    const auto current = clock::now();
    const auto frame_time = time::TimeDelta::from_microseconds(
        std::chrono::duration_cast<std::chrono::microseconds>(current -
                                                              previous)
            .count());
    previous = current;

    accumulator += frame_time;

    constexpr int kMaxIterations = 5;
    int iterations = 0;

    while (accumulator >= timestep && iterations < kMaxIterations) {
      event_bus_->FlushChannel(util::ThreadId::kLogic);
      FlushNetworkCommandQueue();
      registry_->UpdateSystems(timestep);
      ProduceRenderSnapshot();

      accumulator -= timestep;
      ++iterations;
    }

    std::this_thread::sleep_for(std::chrono::microseconds(100));
  }
}

void GameRuntime::FlushNetworkCommandQueue() {
  net::ReceivedPacket packet;
  while (network_in_queue_.TryPop(packet)) {
    event_bus_->Publish(net::PacketReceivedEvent{std::move(packet)});
  }
}

void GameRuntime::ProduceRenderSnapshot() {
  static std::uint32_t tick_count = 0;
  snapshot_buffer_->Produce(render::ExtractSnapshot(*registry_, tick_count++));
}

void GameRuntime::NetworkThreadMain() {
  std::array<std::uint8_t, 4096> receive_buffer;

  while (running_.load(std::memory_order_acquire)) {
    if (!socket_ || !socket_->is_open()) {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      continue;
    }

    auto result = socket_->receive_from(receive_buffer);

    if (result.error) {
      if (result.error != std::errc::resource_unavailable_try_again &&
          result.error != std::errc::operation_would_block) {
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
      continue;
    }

    if (result.bytes_transferred > 0) {
      net::ReceivedPacket packet;
      packet.payload.assign(receive_buffer.begin(),
                            receive_buffer.begin() + result.bytes_transferred);
      packet.remote = result.remote_endpoint;
      network_in_queue_.Push(std::move(packet));
    }
  }
}

void GameRuntime::AudioThreadMain() {
  try {
    [[maybe_unused]] auto audio_engine = audio::CreateRaylibAudioEngine();

    while (running_.load(std::memory_order_acquire)) {
      audio::AudioCommand cmd;
      while (audio_command_queue_.TryPop(cmd)) {
        std::visit(
            [&](auto&& command) {
              using T = std::decay_t<decltype(command)>;
              if constexpr (std::is_same_v<T, audio::PlaySoundCommand>) {
                audio_engine->PlaySoundEffect(command.path);
              } else if constexpr (std::is_same_v<T, audio::PlayMusicCommand>) {
                audio_engine->PlayMusic(command.path);
              } else if constexpr (std::is_same_v<T, audio::StopMusicCommand>) {
                audio_engine->StopMusic();
              } else if constexpr (std::is_same_v<T, audio::SetVolumeCommand>) {
                switch (command.target) {
                  case audio::SetVolumeCommand::Target::kMaster:
                    audio_engine->SetMasterVolume(command.volume);
                    break;
                  case audio::SetVolumeCommand::Target::kMusic:
                    audio_engine->SetMusicVolume(command.volume);
                    break;
                  case audio::SetVolumeCommand::Target::kSfx:
                    audio_engine->SetSfxVolume(command.volume);
                    break;
                }
              }
            },
            cmd);
      }

      audio_engine->Update();
      std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }
  } catch (...) {
  }
}

void GameRuntime::DebugThreadMain() {
  while (running_.load()) {
    std::this_thread::sleep_for(std::chrono::seconds(1));

    if (!frame_profiler_) continue;

    float fps = frame_profiler_->current_fps();
    float latency = 0.0f;
    if (network_debugger_) {
      latency = network_debugger_->conditions().latency_ms.load();
    }

    ENGINE_LOG_INFO("Telemetry: FPS: ", fps, ", Latency (Sim): ", latency,
                    " ms");
  }
}

}  // namespace engine
