#ifndef PROTOCOL_HEADER_H_
#define PROTOCOL_HEADER_H_


#include <cstdint>

#include "protocol/message_type.h"
#include "engine/net/packet_buffer.h"

namespace protocol {

/**
 * @brief Current protocol version for compatibility checks.
 */
inline constexpr std::uint16_t kProtocolVersion = 1;

/**
 * @brief Common header present at the beginning of every R-Type UDP packet.
 * 
 * This structure describes the logical fields of the header. It is NOT
 * meant to be sent directly with memcpy; serialization and deserialization
 * must go through engine::net::PacketBuffer.
 */
struct Header {
  std::uint16_t version;        ///< Protocol version.
  std::uint8_t message_type;    ///< MessageType as raw uint8 on the wire.
  std::uint8_t flags;           ///< Bitfield (reliable, compressed, etc.).
  std::uint32_t sequence;       ///< Sender packet sequence.
  std::uint32_t ack;            ///< Last received sequence from peer.
  std::uint32_t ack_bits;       ///< Bitmap for the previous 32 packets.
  std::uint32_t timestamp_ms;   ///< Timestamp for ping/latency in ms.
};

/**
 * @brief Flags used in Header::flags.
 */
enum HeaderFlag : std::uint8_t {
  kHeaderFlagReliable = 1u << 0,       ///< Packet carries at least one reliable msg.
  kHeaderFlagCompressed = 1u << 1,     ///< Payload is compressed (e.g., LZ4).
  kHeaderFlagConnectionless = 1u << 2  ///< Packet is not bound to a session.
  ///< Bits 3..7 reserved for future use.
};

/**
 * @brief Serializes a Header into a PacketBuffer.
 * @param header The header to serialize.
 * @param writer The packet buffer to write to.
 * @return true on success, false if the writer runs out of space.
 */
bool EncodeHeader(const Header& header, engine::net::PacketBuffer& writer);

/**
 * @brief Deserializes a Header from a PacketBuffer.
 * @param reader The packet buffer to read from.
 * @param out_header Output parameter for the deserialized header.
 * @return true on success, false if the buffer is too small or invalid.
 */
bool DecodeHeader(engine::net::PacketBuffer& reader, Header& out_header);

}  // namespace protocol


#endif // !PROTOCOL_HEADER_H_
