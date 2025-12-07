#include <iostream>

#include "application.h"
#include "client_config.h"

int main(int argc, char** argv) {
  const auto parse_result = client::ParseClientConfig(argc, argv);
  if (!parse_result.ok) {
    std::cerr << parse_result.error << '\n';
    std::cerr << "Usage: " << argv[0]
              << " [--host <host>] [--port <port>] [--debug]\n";
    return 1;
  }

  client::Application app(parse_result.config);
  return app.Run();
}
