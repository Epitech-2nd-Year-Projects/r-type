#ifndef SERVER_RUNTIME_HELPERS_H_
#define SERVER_RUNTIME_HELPERS_H_

#include <chrono>
#include <cstdint>
#include <limits>
#include <string>

#include "engine/net/endpoint.h"
#include "peer_connection.h"

namespace server::runtime_helpers {

inline std::string EndpointKey(const engine::net::Endpoint& endpoint) {
  std::string key = endpoint.address();
  key.append(":");
  key.append(std::to_string(endpoint.port()));
  return key;
}

inline std::uint32_t NowMilliseconds() {
  using namespace std::chrono;
  const auto now = steady_clock::now().time_since_epoch();
  return static_cast<std::uint32_t>(duration_cast<milliseconds>(now).count());
}

inline std::uint32_t ElapsedMilliseconds(std::uint32_t from,
                                         std::uint32_t to) {
  return to >= from
             ? to - from
             : (std::numeric_limits<std::uint32_t>::max() - from) + 1u + to;
}

inline const char* PeerStateToString(PeerState state) {
  switch (state) {
    case PeerState::kConnecting:
      return "connecting";
    case PeerState::kJoined:
      return "joined";
    case PeerState::kDisconnected:
      return "disconnected";
  }
  return "unknown";
}

}  // namespace server::runtime_helpers

#endif  // SERVER_RUNTIME_HELPERS_H_
