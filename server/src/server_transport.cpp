#include "server_transport.h"

#include <array>
#include <utility>

namespace server {

namespace {

bool IsTransientError(const std::error_code& ec) {
  return ec == asio::error::would_block || ec == asio::error::try_again ||
         ec == asio::error::interrupted;
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
  return socket_.send_to(data, size, to);
}

engine::net::UdpSendResult ServerTransport::Send(
    const UdpEndpoint& to, const engine::net::PacketBuffer& buffer) {
  return Send(to, buffer.data(), buffer.size());
}

ServerTransport::PollResult ServerTransport::PollNetwork() {
  PollResult result{};
  if (!running_) return result;

  std::array<std::uint8_t, kMaxDatagramSize> buffer{};

  while (true) {
    const auto recv_result =
        socket_.receive_from(buffer.data(), buffer.size());
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
    datagram.payload = engine::net::PacketBuffer(
        buffer.data(), recv_result.bytes_transferred);
    result.datagrams.push_back(std::move(datagram));
  }
  return result;
}

}  // namespace server
