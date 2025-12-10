#ifndef CLIENT_APPLICATION_H_
#define CLIENT_APPLICATION_H_

#include <memory>

#include "audio_manager.h"
#include "client_config.h"
#include "join_flow.h"
#include "network_transport.h"
#include "input_layer.h"
#include "input_sender.h"
#include "world_update_receiver.h"
#include "engine/core/engine_runtime.h"
#include "engine/time/time_delta.h"

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

 private:
  bool Tick(engine::time::TimeDelta dt);
  void UpdateAudio(engine::time::TimeDelta dt, JoinState join_state);
  void HandleGameOverAudio();

  ClientConfig config_;
  std::shared_ptr<NetworkTransport> transport_;
  JoinFlow join_flow_;
  std::unique_ptr<engine::core::EngineRuntime> engine_;
  std::unique_ptr<InputLayer> input_layer_;
  std::unique_ptr<InputSender> input_sender_;
  std::unique_ptr<AudioManager> audio_manager_;
  WorldUpdateReceiver world_update_receiver_;
  JoinState last_join_state_{JoinState::kIdle};
  bool music_allowed_{false};
  bool music_blocked_{false};
};

}  // namespace client

#endif  // CLIENT_APPLICATION_H_
