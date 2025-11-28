#ifndef PING_H_
#define PING_H_

#include <cstdint>
#include "engine/net/packet_buffer.h"

namespace protocol {
  /**
   * @brief Payload structure for ping messages.
   * 
   * Contains a timestamp to measure round-trip time.
   */
  struct PingPayload {
    std::uint32_t client_time_ms = 0;  ///< Client timestamp in milliseconds
  };

  /**
   * @brief Payload structure for pong messages.
   * 
   * Echoes the timestamp received in the ping.
   */
  struct PongPayload {
    std::uint32_t client_time_ms = 0;   ///< Echoed client timestamp in milliseconds
    std::uint32_t server_time_ms = 0;   ///< Server timestamp in milliseconds
  };

  /**
   * @brief Serializes a PingPayload into a PacketBuffer.
   * @param ping The ping payload to serialize.
   * @param writer The packet buffer to write to.
   */
  bool EncodePing(const PingPayload& ping, engine::net::PacketBuffer& writer);
  
  /**
   * @brief Deserializes a PingPayload from a PacketBuffer.
   * @param reader The packet buffer to read from.
   * @param out_ping Output parameter for the deserialized ping payload.
   * @return true on success, false if the buffer is too small or invalid.
   */
  bool DecodePing(engine::net::PacketBuffer& reader, PingPayload& out_ping);

  /**
   * @brief Serializes a PongPayload into a PacketBuffer.
   * @param pong The pong payload to serialize.
   * @param writer The packet buffer to write to.
   */
  bool EncodePong(const PongPayload& pong, engine::net::PacketBuffer& writer);

  /**
   * @brief Deserializes a PongPayload from a PacketBuffer.
   * @param reader The packet buffer to read from.
   * @param out_pong Output parameter for the deserialized pong payload.
   * @return true on success, false if the buffer is too small or invalid.
   */
  bool DecodePong(engine::net::PacketBuffer& reader, PongPayload& out_pong);

}


#endif // !PING_H_
