#ifndef CLIENT_APPLICATION_H_
#define CLIENT_APPLICATION_H_

#include <memory>

#include "engine/core/engine_runtime.h"
#include "engine/time/time_delta.h"

namespace client {

/**
 * @brief High-level application object driving the client runtime
 */
class Application {
 public:
  Application();
  int Run();

 private:
  bool Tick(engine::time::TimeDelta dt);

  std::unique_ptr<engine::core::EngineRuntime> engine_;
};

}  // namespace client

#endif  // CLIENT_APPLICATION_H_
