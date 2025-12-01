#ifndef CLIENT_CLIENT_APP_H_
#define CLIENT_CLIENT_APP_H_

#include <memory>

#include "engine/audio/audio_engine.h"
#include "engine/ecs/registry.h"
#include "engine/input.h"
#include "engine/net/udp_socket.h"
#include "engine/render/window.h"
#include "engine/time/game_loop.h"

namespace client {

/**
 * @class ClientApp
 * @brief Boots client-facing engine subsystems and runs the main loop
 *
 * Owns rendering, audio, networking, input, ECS, and time management required
 * by the client runtime, exposing a single Run() entrypoint.
 */
class ClientApp {
 public:
  /**
   * @brief Construct an uninitialized client application shell
   */
  ClientApp();

  /**
   * @brief Clean up owned subsystems
   */
  ~ClientApp();

  /**
   * @brief Initialize subsystems and execute the main loop
   * @return Exit code suitable for process return
   */
  int Run();

 private:
  /**
   * @brief Prepare window, audio, networking, and default bindings
   * @return true when all subsystems are ready
   */
  bool Initialize();

  /**
   * @brief Single frame of the main loop
   * @param dt Delta time since last frame
   * @return false to exit loop
   */
  bool Tick(engine::time::TimeDelta dt);

  /**
   * @brief Collect raw device input and emit high-level actions
   */
  void PumpInput();

  /**
   * @brief Handle incoming datagrams on the UDP socket
   */
  void ServiceNetwork();

  /**
   * @brief Render the current frame
   */
  void RenderFrame();

  /**
   * @brief Advance audio streaming and cleanup
   */
  void UpdateAudio();

  engine::render::WindowConfig window_config_;
  engine::time::VariableTimestepLoop loop_;
  engine::ecs::Registry registry_;
  engine::input::InputManager input_manager_;
  std::unique_ptr<engine::render::WindowBackend> render_backend_;
  std::unique_ptr<engine::render::Window> window_;
  std::unique_ptr<engine::audio::AudioEngine> audio_engine_;
  engine::net::UdpSocket net_socket_;
  bool should_exit_{false};
};

}  // namespace client

#endif  // CLIENT_CLIENT_APP_H_
