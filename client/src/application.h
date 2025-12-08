#ifndef CLIENT_APPLICATION_H_
#define CLIENT_APPLICATION_H_

#include <memory>

#include "client_config.h"
#include "audio_manager.h"
#include "join_flow.h"
#include "network_transport.h"
#include "engine/core/engine_runtime.h"
#include "engine/time/time_delta.h"

namespace client {

/**
 * @brief High-level states of the application
 */
enum class GameState {
  kMainMenu,
  kConnecting,
  kInGame
};

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

 private:
  bool Tick(engine::time::TimeDelta dt);

  ClientConfig config_;
  NetworkTransport transport_;
  JoinFlow join_flow_;
  GameState state_{GameState::kMainMenu};
  std::unique_ptr<engine::core::EngineRuntime> engine_;
  std::unique_ptr<AudioManager> audio_manager_;
};

}  // namespace client

#endif  // CLIENT_APPLICATION_H_
