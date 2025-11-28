#include "protocol/header.h"

namespace protocol {

void EncodeHeader(const Header& header, engine::net::PacketBuffer& writer) {
  writer.WriteUint16(header.version);
  writer.WriteUint8(header.message_type);
  writer.WriteUint8(header.flags);
  writer.WriteUint32(header.sequence);
  writer.WriteUint32(header.ack);
  writer.WriteUint32(header.ack_bits);
  writer.WriteUint32(header.timestamp_ms);
}

bool DecodeHeader(engine::net::PacketBuffer& reader, Header& out_header) {
  std::uint16_t version;
  std::uint8_t message_type;
  std::uint8_t flags;
  std::uint32_t sequence;
  std::uint32_t ack;
  std::uint32_t ack_bits;
  std::uint32_t timestamp_ms;

  if (!reader.ReadUint16(version) || !reader.ReadUint8(message_type) ||
      !reader.ReadUint8(flags) || !reader.ReadUint32(sequence) ||
      !reader.ReadUint32(ack) || !reader.ReadUint32(ack_bits) ||
      !reader.ReadUint32(timestamp_ms)) {
    return false;
  }
  out_header.version = version;
  out_header.message_type = message_type;
  out_header.flags = flags;
  out_header.sequence = sequence;
  out_header.ack = ack;
  out_header.ack_bits = ack_bits;
  out_header.timestamp_ms = timestamp_ms;
  return true;
}
}  // namespace protocol
