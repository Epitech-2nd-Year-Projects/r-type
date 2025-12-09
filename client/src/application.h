#ifndef CLIENT_APPLICATION_H_
#define CLIENT_APPLICATION_H_

#include <memory>

#include "audio_manager.h"
#include "client_config.h"
#include "join_flow.h"
#include "network_transport.h"
#include "input_layer.h"
#include "input_sender.h"
#include "engine/core/engine_runtime.h"
#include "engine/time/time_delta.h"
#include "protocol/sequence_tracker.h"

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

  ClientConfig config_;
  NetworkTransport transport_;
  std::shared_ptr<protocol::SequenceTracker> sequence_tracker_{};
  JoinFlow join_flow_;
  std::unique_ptr<engine::core::EngineRuntime> engine_;
  std::unique_ptr<InputLayer> input_layer_;
  std::unique_ptr<InputSender> input_sender_;
  std::unique_ptr<AudioManager> audio_manager_;
};

}  // namespace client

#endif  // CLIENT_APPLICATION_H_
