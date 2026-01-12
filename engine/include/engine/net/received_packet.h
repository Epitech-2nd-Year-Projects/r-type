#ifndef ENGINE_NET_RECEIVED_PACKET_H_
#define ENGINE_NET_RECEIVED_PACKET_H_

#include <cstdint>
#include <vector>

#include "engine/net/endpoint.h"

namespace engine::net {

/**
 * @brief Represents a raw packet received from the network.
 *
 * The engine does not know the content of the packet (Protocol Agnostic).
 * It is up to the game logic (systems) to decode it.
 */
struct ReceivedPacket {
  std::vector<std::uint8_t> payload;
  Endpoint remote;
};

}  // namespace engine::net

#endif  // ENGINE_NET_RECEIVED_PACKET_H_
