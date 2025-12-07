#include "network_transport.h"

#include <system_error>

namespace client {

std::error_code NetworkTransport::Start(
    std::string_view host, std::uint16_t port,
    std::optional<std::uint16_t> local_port) {
  std::error_code resolve_error;
  const auto server = engine::net::Endpoint::Resolve(host, port, resolve_error);
  if (resolve_error) return resolve_error;
  if (!server.valid()) return std::make_error_code(std::errc::invalid_argument);

  std::optional<engine::net::Endpoint> bind_endpoint;
  if (local_port.has_value()) {
    bind_endpoint = server.is_ipv6()
                        ? engine::net::Endpoint::AnyIpv6(*local_port)
                        : engine::net::Endpoint::AnyIpv4(*local_port);
  }

  return client_.Start(server, bind_endpoint);
}

void NetworkTransport::Stop() { client_.Stop(); }

bool NetworkTransport::Send(engine::net::PacketBuffer packet) {
  return client_.Enqueue(std::move(packet));
}

bool NetworkTransport::Receive(
    engine::net::Client::ReceivedPacket& out_packet) {
  return client_.TryDequeue(out_packet);
}

}  // namespace client
