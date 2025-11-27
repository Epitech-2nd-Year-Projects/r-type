#ifndef HEADER_H_
#define HEADER_H_


#include <cstdint>

#include "protocol/message_type.h"

namespace engine::net {
  class BufferWriter;
  class BufferReader;
}

namespace protocol {

inline constexpr std::uint16_t kProtocolVersion = 1;

struct Header {
  std::uint16_t version;
  std::uint8_t message_type;
  std::uint8_t flags;
  std::uint32_t sequence;
  std::uint32_t ack;
  std::uint32_t ack_bits;
  std::uint32_t timestamp_ms;
};

enum HeaderFlag : std::uint8_t {
  kHeaderFlagReliable = 1u << 0,
  kHeaderFlagCompressed = 1u << 1,
  kHeaderFlagConnectionless = 1u << 2
};

bool EncodeHeader(const Header& header, engine::net::BufferWriter& writer);

bool DecodeHeader(engine::net::BufferReader& reader, Header& out_header);

}


#endif /* !HEADER_H_ */
