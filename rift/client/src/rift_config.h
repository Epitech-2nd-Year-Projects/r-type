#ifndef RIFT_CLIENT_RIFT_CONFIG_H_
#define RIFT_CLIENT_RIFT_CONFIG_H_

#include <cstdint>
#include <string>

namespace rift::client {

struct RiftConfig {
  std::string host{"127.0.0.1"};
  std::uint16_t port{4243};
  std::string player_name{"Fighter"};
  bool debug{false};
  std::uint32_t timeout_ms{7000};
  std::uint32_t ping_interval_ms{1000};
  std::uint32_t join_retry_delay_ms{500};
  int join_max_attempts{5};
  int resolution_width{1280};
  int resolution_height{720};
  bool fullscreen{false};
  bool vsync{true};
  int target_fps{60};
};

RiftConfig LoadRiftConfig(int argc, char** argv);

}  // namespace rift::client

#endif  // RIFT_CLIENT_RIFT_CONFIG_H_
