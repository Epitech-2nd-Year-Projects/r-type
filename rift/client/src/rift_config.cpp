#include "rift_config.h"

#include <cstdlib>
#include <cstring>

namespace rift::client {

RiftConfig LoadRiftConfig(int argc, char** argv) {
  RiftConfig config;

  for (int i = 1; i < argc; ++i) {
    if (std::strcmp(argv[i], "--host") == 0 && i + 1 < argc) {
      config.host = argv[++i];
    } else if (std::strcmp(argv[i], "--port") == 0 && i + 1 < argc) {
      config.port = static_cast<std::uint16_t>(std::atoi(argv[++i]));
    } else if (std::strcmp(argv[i], "--name") == 0 && i + 1 < argc) {
      config.player_name = argv[++i];
    } else if (std::strcmp(argv[i], "--debug") == 0) {
      config.debug = true;
    } else if (std::strcmp(argv[i], "--fullscreen") == 0) {
      config.fullscreen = true;
    } else if (std::strcmp(argv[i], "--width") == 0 && i + 1 < argc) {
      config.resolution_width = std::atoi(argv[++i]);
    } else if (std::strcmp(argv[i], "--height") == 0 && i + 1 < argc) {
      config.resolution_height = std::atoi(argv[++i]);
    }
  }

  if (const char* env_host = std::getenv("RIFT_CLIENT_HOST")) {
    config.host = env_host;
  }
  if (const char* env_port = std::getenv("RIFT_CLIENT_PORT")) {
    config.port = static_cast<std::uint16_t>(std::atoi(env_port));
  }
  if (const char* env_name = std::getenv("RIFT_CLIENT_NAME")) {
    config.player_name = env_name;
  }

  return config;
}

}  // namespace rift::client
