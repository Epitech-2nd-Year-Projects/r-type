#include "engine/game_runtime.h"

#include <chrono>

namespace engine {

GameRuntime::GameRuntime()
    : config_(Config{}),
      registry_(std::make_unique<ecs::Registry>()),
      event_bus_(std::make_unique<event::EventBus>()),
      snapshot_buffer_(std::make_unique<render::SnapshotBuffer>()),
      frame_interpolator_(
          std::make_unique<render::FrameInterpolator>(*snapshot_buffer_)) {}

GameRuntime::GameRuntime(Config config)
    : config_(config),
      registry_(std::make_unique<ecs::Registry>()),
      event_bus_(std::make_unique<event::EventBus>()),
      snapshot_buffer_(std::make_unique<render::SnapshotBuffer>()),
      frame_interpolator_(
          std::make_unique<render::FrameInterpolator>(*snapshot_buffer_)) {}

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

ecs::Registry& GameRuntime::Registry() { return *registry_; }

event::EventBus& GameRuntime::EventBus() { return *event_bus_; }

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

void GameRuntime::FlushNetworkCommandQueue() {}

void GameRuntime::ProduceRenderSnapshot() {
  static std::uint32_t tick_count = 0;
  snapshot_buffer_->Produce(render::ExtractSnapshot(*registry_, tick_count++));
}

void GameRuntime::NetworkThreadMain() {
  while (running_.load()) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
}

void GameRuntime::AudioThreadMain() {
  while (running_.load()) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
}

void GameRuntime::DebugThreadMain() {
  while (running_.load()) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
}

}  // namespace engine
