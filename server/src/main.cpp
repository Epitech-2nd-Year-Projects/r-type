#include "admin_console.h"
#include "engine/util/logging.h"
#include "server_config.h"
#include "server_runtime.h"

int main(int argc, char** argv) {
  (void)argc;
  (void)argv;

  server::ServerConfig config = server::LoadServerConfig();
  server::ServerRuntime runtime(config);

  if (const auto start_error = runtime.Start(); start_error) {
    engine::util::Logger::Default().Error("[server] Failed to start server: ",
                                          start_error.message());
    return 1;
  }

  server::AdminConsole console(runtime);
  console.Start();

  runtime.RunMainLoop();

  console.Stop();
  return 0;
}
