#ifndef PROTOCOL_PACKET_H_
#define PROTOCOL_PACKET_H_

#include <variant>
#include "engine/net/packet_buffer.h"
#include "protocol/command.h"
#include "protocol/header.h"
#include "protocol/input_state.h"
#include "protocol/join.h"
#include "protocol/ping.h"
#include "protocol/player_died.h"
#include "protocol/world_snapshot.h"


namespace protocol {

/**
 * @brief Variant type holding all possible packet payload types.
 * 
 * Uses std::variant to provide type-safe storage for any message payload.
 * std::monostate represents packets with no payload.
 */
using PacketPayload = std::variant<std::monostate,
                                    InputStatePayload,
                                    PingPayload,
                                    PongPayload,
                                    JoinRequestPayload,
                                    JoinAcceptPayload,
                                    JoinRejectPayload,
                                    PlayerDiedPayload,
                                    CommandPayload,
                                    WorldSnapshotPayload>;

  /**
   * @brief Complete packet structure with header and payload.
   * 
   * Represents a full UDP packet in the R-Type protocol,
   * consisting of a common header and a type-specific payload.
   */
  struct Packet {
    Header header;          ///< Common packet header with metadata.
    PacketPayload payload;  ///< Type-specific payload data.
  };

  /**
   * @brief Encodes a complete packet (header + payload) into a PacketBuffer.
   * @param packet The packet to serialize.
   * @param buffer The packet buffer to write to.
   * @return true on success, false if encoding fails or payload type doesn't match header.
   */
  bool EncodePacket(const Packet& packet, engine::net::PacketBuffer& buffer);

  /**
   * @brief Decodes a complete packet (header + payload) from a PacketBuffer.
   * @param buffer The packet buffer to read from.
   * @param out_packet Output parameter for the deserialized packet.
   * @return true on success, false if the buffer is invalid or decoding fails.
   */
  bool DecodePacket(engine::net::PacketBuffer& buffer, Packet& out_packet);
}

#endif  // PROTOCOL_PACKET_H_
