#include "../../include/engine/net/udp_socket.h"

namespace engine::net {

namespace {

std::error_code MakeNotOpenError() {
  return std::make_error_code(std::errc::bad_file_descriptor);
}

}  // namespace

UdpSocket::UdpSocket(Protocol protocol)
    : owned_context_(std::make_shared<asio::io_context>()),
      context_ref_(*owned_context_),
      socket_(std::make_unique<asio::ip::udp::socket>(context_ref_.get())) {
  open(protocol);
}

UdpSocket::UdpSocket(asio::io_context& external_context, Protocol protocol)
    : owned_context_(nullptr),
      context_ref_(external_context),
      socket_(std::make_unique<asio::ip::udp::socket>(external_context)) {
  open(protocol);
}

UdpSocket::~UdpSocket() { close(); }

std::error_code UdpSocket::open(Protocol protocol) {
  if (!socket_) {
    socket_ = std::make_unique<asio::ip::udp::socket>(context_ref_.get());
  }
  std::error_code ec;
  if (socket_->is_open()) {
    socket_->close(ec);
    if (ec) return ec;
  }

  const auto proto =
      protocol == Protocol::kIpv4 ? asio::ip::udp::v4() : asio::ip::udp::v6();
  socket_->open(proto, ec);
  if (ec) return ec;

  std::error_code nb_error;
  socket_->non_blocking(true, nb_error);
  if (nb_error) ec = nb_error;
  has_connected_endpoint_ = false;
  connected_endpoint_ = Endpoint();
  return ec;
}

std::error_code UdpSocket::bind(const Endpoint& endpoint) {
  if (!socket_) return MakeNotOpenError();
  std::error_code ec;
  socket_->bind(endpoint.native(), ec);
  return ec;
}

std::error_code UdpSocket::connect(const Endpoint& endpoint) {
  if (!socket_) return MakeNotOpenError();
  std::error_code ec;
  socket_->connect(endpoint.native(), ec);
  if (!ec) {
    connected_endpoint_ = endpoint;
    has_connected_endpoint_ = true;
  }
  return ec;
}

UdpSendResult UdpSocket::send(std::span<const std::uint8_t> data) {
  if (!socket_) return {0, MakeNotOpenError()};
  UdpSendResult result{};
  asio::error_code ec;
  result.bytes_transferred =
      socket_->send(asio::buffer(data.data(), data.size()), 0, ec);
  result.error = ec;
  return result;
}

UdpSendResult UdpSocket::send_to(std::span<const std::uint8_t> data,
                                 const Endpoint& endpoint) {
  if (!socket_) return {0, MakeNotOpenError()};
  UdpSendResult result{};
  asio::error_code ec;
  result.bytes_transferred =
      socket_->send_to(asio::buffer(data.data(), data.size()),
                       endpoint.native(), 0, ec);
  result.error = ec;
  return result;
}

UdpReceiveResult UdpSocket::receive(std::span<std::uint8_t> data) {
  if (!socket_) return {0, Endpoint(), MakeNotOpenError()};
  UdpReceiveResult result{};
  asio::error_code ec;
  result.bytes_transferred =
      socket_->receive(asio::buffer(data.data(), data.size()), 0, ec);
  result.error = ec;
  if (!ec && has_connected_endpoint_)
    result.remote_endpoint = connected_endpoint_;
  return result;
}

UdpReceiveResult UdpSocket::receive_from(std::span<std::uint8_t> data) {
  if (!socket_) return {0, Endpoint(), MakeNotOpenError()};
  UdpReceiveResult result{};
  asio::error_code ec;
  asio::ip::udp::endpoint sender;
  result.bytes_transferred =
      socket_->receive_from(asio::buffer(data.data(), data.size()), sender, 0,
                            ec);
  result.error = ec;
  if (!ec) result.remote_endpoint = Endpoint(sender, true);
  return result;
}

void UdpSocket::close() {
  if (!socket_) return;
  std::error_code ec;
  socket_->close(ec);
  has_connected_endpoint_ = false;
}

}  // namespace engine::net
