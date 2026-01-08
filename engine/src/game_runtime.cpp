#include "engine/game_runtime.h"

#include <chrono>

namespace engine {

GameRuntime::GameRuntime() : GameRuntime(Config{}) {}

GameRuntime::GameRuntime(Config config)
    : config_(config),
      registry_(std::make_unique<ecs::Registry>()),
      event_bus_(std::make_unique<event::EventBus>()) {}

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
  while (running_.load()) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
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
