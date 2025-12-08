#ifndef CLIENT_APPLICATION_H_
#define CLIENT_APPLICATION_H_

#include <memory>

#include "audio_manager.h"
#include "client_config.h"
#include "audio_manager.h"
#include "join_flow.h"
#include "network_transport.h"
#include "engine/core/engine_runtime.h"
#include "engine/time/time_delta.h"
#include "scene/scene.h"

namespace client {

/**
 * @brief High-level application object driving the client runtime
 */
class Application {
 public:
  /**
   * @brief Construct an Application with user provided configuration
   */
  explicit Application(ClientConfig config);
  int Run();

  /**
   * @brief Access the engine runtime
   */
  engine::core::EngineRuntime& GetEngine() { return *engine_; }

  /**
   * @brief Switch the active scene
   */
  void SwitchScene(std::unique_ptr<Scene> scene);

  void StartConnection();
  void OnConnected();
  void OnConnectionFailed(const std::string& reason);
  void OnGameStart();
  void OnGamePause();
  void OnGameResume();
  void OnGameOver();
  void OnDisconnect();
  void OnQuitToMenu();

  JoinFlow& GetJoinFlow() { return join_flow_; }
  NetworkTransport& GetTransport() { return transport_; }

 private:
  bool Tick(engine::time::TimeDelta dt);

  ClientConfig config_;
  NetworkTransport transport_;
  JoinFlow join_flow_;
  std::unique_ptr<Scene> current_scene_;
  std::unique_ptr<engine::core::EngineRuntime> engine_;
  std::unique_ptr<AudioManager> audio_manager_;
};

}  // namespace client

#endif  // CLIENT_APPLICATION_H_
