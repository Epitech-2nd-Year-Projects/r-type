#ifndef CLIENT_CLIENT_CONFIG_H_
#define CLIENT_CLIENT_CONFIG_H_

#include <cstdint>
#include <string>

namespace client {

/**
 * @brief User provided configuration for the client runtime
 */
struct ClientConfig {
  std::string host{"127.0.0.1"};
  std::uint16_t port{4242};
  bool debug{false};
};

/**
 * @brief Outcome of parsing command line arguments into a ClientConfig
 */
struct ClientConfigParseResult {
  ClientConfig config{};
  bool ok{true};
  std::string error;
};

/**
 * @brief Parse CLI flags into a ClientConfig instance
 */
ClientConfigParseResult ParseClientConfig(int argc, char** argv);

}  // namespace client

#endif  // CLIENT_CLIENT_CONFIG_H_
