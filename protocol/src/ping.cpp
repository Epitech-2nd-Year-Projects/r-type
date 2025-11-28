#include "protocol/ping.h"

namespace protocol {

bool EncodePing(const PingPayload& ping, engine::net::PacketBuffer& writer) {
  writer.WriteUint32(ping.client_time_ms);
  return true;
}

bool DecodePing(engine::net::PacketBuffer& reader, PingPayload& out_ping) {
  std::uint32_t client_time_ms;
  if (!reader.ReadUint32(client_time_ms)) {
    return false;
  }
  out_ping.client_time_ms = client_time_ms;
  return true;
}

bool EncodePong(const PongPayload& pong, engine::net::PacketBuffer& writer) {
  writer.WriteUint32(pong.client_time_ms);
  writer.WriteUint32(pong.server_time_ms);
  return true;
}

bool DecodePong(engine::net::PacketBuffer& reader, PongPayload& out_pong) {
  std::uint32_t client_time_ms;
  std::uint32_t server_time_ms;
  if (!reader.ReadUint32(client_time_ms) ||
      !reader.ReadUint32(server_time_ms)) {
    return false;
  }
  out_pong.client_time_ms = client_time_ms;
  out_pong.server_time_ms = server_time_ms;
  return true;
}

}  // namespace protocol
