#include <iostream>
#include <span>
#include <string_view>
#include <vector>

#include "server_config.h"
#include "server_runtime.h"

int main(int argc, char** argv) {
  std::vector<std::string_view> args;
  args.reserve(static_cast<std::size_t>(argc));
  for (int i = 0; i < argc; ++i) {
    args.emplace_back(argv[i]);
  }

  const auto parse_result =
      server::ParseServerConfig(std::span(args.data(), args.size()));
  if (!parse_result.ok) {
    std::cerr << parse_result.error << '\n';
    std::cerr << "Usage: " << argv[0] << " [--port <port>] [--max-players <n>]"
              << " [--tickrate|--tick-rate <hz>]"
              << " [--timeout-ms <ms>] [--room-idle-timeout-ms <ms>]"
              << " [--room|--room-code <code>] [--seed <seed>] "
              << "[--log-level <level>]\n";
    return 1;
  }

  server::ServerRuntime runtime(parse_result.config);
  if (const auto start_error = runtime.Start(); start_error) {
    std::cerr << "Failed to start server: " << start_error.message() << '\n';
    return 1;
  }

  runtime.RunMainLoop();
  return 0;
}
