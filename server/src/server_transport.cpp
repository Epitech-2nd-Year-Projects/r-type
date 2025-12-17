#include "server_transport.h"

#include <array>
#include <asio.hpp>
#include <utility>

namespace server {

namespace {

bool IsTransientError(const std::error_code& ec) {
  return ec == asio::error::would_block || ec == asio::error::try_again ||
         ec == asio::error::interrupted ||
         ec == asio::error::connection_reset;
}

}  // namespace

std::error_code ServerTransport::Start(std::uint16_t port) {
  Stop();

  const auto bind_endpoint = UdpEndpoint::AnyIpv4(port);
  const auto open_error = socket_.open(engine::net::UdpSocket::Protocol::kIpv4);
  if (open_error) {
    running_ = false;
    return open_error;
  }
  if (const auto bind_error = socket_.bind(bind_endpoint); bind_error) {
    running_ = false;
    return bind_error;
  }
  bound_endpoint_ = bind_endpoint;
  running_ = true;
  return {};
}

void ServerTransport::Stop() {
  running_ = false;
  socket_.close();
  bound_endpoint_ = UdpEndpoint();
}

engine::net::UdpSendResult ServerTransport::Send(const UdpEndpoint& to,
                                                 const void* data,
                                                 std::size_t size) {
  if (!running_) {
    return {0, std::make_error_code(std::errc::not_connected)};
  }
  const auto* bytes = static_cast<const std::uint8_t*>(data);
  return socket_.send_to(std::span(bytes, size), to);
}

engine::net::UdpSendResult ServerTransport::Send(
    const UdpEndpoint& to, const engine::net::PacketBuffer& buffer) {
  if (!running_) {
    return {0, std::make_error_code(std::errc::not_connected)};
  }
  return socket_.send_to(buffer.data(), to);
}

ServerTransport::PollResult ServerTransport::PollNetwork() {
  PollResult result{};
  if (!running_) return result;

  std::array<std::uint8_t, kMaxDatagramSize> buffer{};

  while (true) {
    const auto recv_result = socket_.receive_from(std::span(buffer));
    if (recv_result.error) {
      if (IsTransientError(recv_result.error)) {
        break;
      }
      result.error = recv_result.error;
      running_ = false;
      break;
    }
    if (recv_result.bytes_transferred == 0) {
      break;
    }
    Datagram datagram{};
    datagram.from = recv_result.remote_endpoint;
    const auto payload_view =
        std::span(buffer).first(recv_result.bytes_transferred);
    datagram.payload = engine::net::PacketBuffer(payload_view);
    result.datagrams.push_back(std::move(datagram));
  }
  return result;
}

}  // namespace server
