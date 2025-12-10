#include <iostream>
#include <span>
#include <string_view>
#include <vector>

#include "application.h"
#include "client_config.h"

int main(int argc, char** argv) {
  std::vector<std::string_view> args;
  args.reserve(static_cast<std::size_t>(argc));
  for (int i = 0; i < argc; ++i) {
    args.emplace_back(argv[i]);
  }

  const auto parse_result =
      client::ParseClientConfig(std::span(args.data(), args.size()));
  if (!parse_result.ok) {
    std::cerr << parse_result.error << '\n';
    std::cerr
        << "Usage: " << argv[0]
        << " [--host <host>] [--port <port>] [--name <player>] [--room <room>]"
        << " [--log-level <level>] [--debug]\n";
    return 1;
  }

  client::Application app(parse_result.config);
  return app.Run();
}
