#ifndef ENGINE_GAME_RUNTIME_H_
#define ENGINE_GAME_RUNTIME_H_

#include <atomic>
#include <functional>
#include <memory>
#include <string_view>
#include <thread>
#include <vector>

#include "engine/audio/audio_command.h"
#include "engine/audio/audio_dispatcher.h"
#include "engine/ecs/registry.h"
#include "engine/event.h"
#include "engine/net/received_packet.h"
#include "engine/net/udp_socket.h"
#include "engine/render/frame_interpolator.h"
#include "engine/render/snapshot_buffer.h"
#include "engine/time/time_delta.h"
#include "engine/util/thread_safe_queue.h"

namespace engine {

/**
 * @brief Central orchestrator for the game engine's runtime lifecycle.
 *
 * This class owns standard modules (Registry, EventBus) and manages the
 * lifecycle of dedicated worker threads (Logic, Network, Audio, Debug).
 * It enforces the "Passive Modules, Active Runtime" architecture.
 */
class GameRuntime {
 public:
  struct Config {
    time::TimeDelta logic_timestep{time::TimeDelta::from_seconds(1.0f / 60.0f)};
    std::string_view window_title{"R-Type"};
  };

  /**
   * @brief Constructs the runtime environment with default configuration.
   */
  GameRuntime();

  /**
   * @brief Constructs the runtime environment with custom configuration.
   */
  explicit GameRuntime(Config config);

  ~GameRuntime();

  GameRuntime(const GameRuntime&) = delete;
  GameRuntime& operator=(const GameRuntime&) = delete;
  GameRuntime(GameRuntime&&) = delete;
  GameRuntime& operator=(GameRuntime&&) = delete;

  /**
   * @brief Starts all worker threads.
   */
  void Start();

  /**
   * @brief Signals all threads to stop and waits for them to join.
   */
  void Stop();

  /**
   * @brief Checks if the runtime uses threads and is currently running.
   */
  [[nodiscard]] bool Running() const;

  /**
   * @brief Access the ECS Registry.
   * @return Reference to the thread-local or shared Registry.
   */
  void StartNetwork(std::uint16_t port);
  [[nodiscard]] std::uint16_t GetBoundPort() const;

  ecs::Registry& Registry();

  /**
   * @brief Access the central EventBus.
   * @return Reference to the thread-safe EventBus.
   */
  [[nodiscard]] event::EventBus& EventBus();

  [[nodiscard]] audio::AudioDispatcher& GetAudioDispatcher();

  /**
   * @brief Executes the main thread loop (typically rendering).
   *
   * @param render_callback Function to be called every frame on the main
   * thread. Should return false to request engine stop.
   */
  void RunMainThread(
      std::function<bool(time::TimeDelta)> render_callback = nullptr);

 private:
  void LogicThreadMain();
  void NetworkThreadMain();
  void AudioThreadMain();
  void DebugThreadMain();

  void FlushNetworkCommandQueue();
  void ProduceRenderSnapshot();

  Config config_;
  std::atomic<bool> running_{false};

  std::unique_ptr<ecs::Registry> registry_;
  std::unique_ptr<event::EventBus> event_bus_;
  std::unique_ptr<render::SnapshotBuffer> snapshot_buffer_;
  std::unique_ptr<render::FrameInterpolator> frame_interpolator_;

  util::ThreadSafeQueue<net::ReceivedPacket> network_in_queue_;
  std::unique_ptr<net::UdpSocket> socket_;

  audio::AudioCommandQueue audio_command_queue_;
  std::unique_ptr<audio::AudioDispatcher> audio_dispatcher_;

  std::unique_ptr<std::thread> logic_thread_;
  std::unique_ptr<std::thread> network_thread_;
  std::unique_ptr<std::thread> audio_thread_;
  std::unique_ptr<std::thread> debug_thread_;
};

}  // namespace engine

#endif  // ENGINE_GAME_RUNTIME_H_
