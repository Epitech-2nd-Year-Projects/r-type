#include "engine/util/logging.h"
#include "server_config.h"
#include "server_runtime.h"

int main(int argc, char** argv) {
  (void)argc;
  (void)argv;

  server::ServerConfig config = server::LoadServerConfig();
  server::ServerRuntime runtime(config);
  if (const auto start_error = runtime.Start(); start_error) {
    engine::util::Logger::Default().Error("[rift_server] Failed to start server: ",
                                          start_error.message());
    return 1;
  }

  runtime.RunMainLoop();
  return 0;
}
