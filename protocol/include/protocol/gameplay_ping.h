#ifndef PROTOCOL_GAMEPLAY_PING_H_
#define PROTOCOL_GAMEPLAY_PING_H_

#include <cstdint>
#include "engine/net/packet_buffer.h"

namespace protocol {

/**
 * @enum PingType
 * @brief Type of gameplay ping.
 */
enum class PingType : std::uint8_t {
  kAttack = 0,
  kDefend = 1,
  kDanger = 2,
  kOnMyWay = 3,
  kGeneric = 4
};

/**
 * @brief Payload for gameplay pings.
 */
struct GameplayPingPayload {
  std::uint32_t sender_id = 0; // Player ID
  PingType type = PingType::kGeneric;
  float x = 0.0f;
  float y = 0.0f;
};

/**
 * @brief Serialize a GameplayPingPayload.
 */
bool EncodeGameplayPing(const GameplayPingPayload& payload, engine::net::PacketBuffer& writer);

/**
 * @brief Deserialize a GameplayPingPayload.
 */
bool DecodeGameplayPing(engine::net::PacketBuffer& reader, GameplayPingPayload& out_payload);

} // namespace protocol

#endif // PROTOCOL_GAMEPLAY_PING_H_
