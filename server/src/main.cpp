#include <iostream>

#include "server_config.h"
#include "server_runtime.h"

int main(int argc, char** argv) {
  const auto parse_result = server::ParseServerConfig(argc, argv);
  if (!parse_result.ok) {
    std::cerr << parse_result.error << '\n';
    std::cerr << "Usage: " << argv[0]
              << " [--port <port>] [--max-players <n>] [--tickrate <hz>]"
              << " [--room <name>] [--seed <seed>] "
              << "[--log-level <level>]\n";
    return 1;
  }

  server::ServerRuntime runtime(parse_result.config);
  if (const auto start_error = runtime.Start(); start_error) {
    std::cerr << "Failed to start server: " << start_error.message() << '\n';
    return 1;
  }

  runtime.Run();
  return 0;
}
