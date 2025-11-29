#include "../../include/engine/net/udp_socket.h"

namespace engine::net {

namespace {

std::shared_ptr<asio::io_context> WrapExternalContext(
    asio::io_context& context) {
  return std::shared_ptr<asio::io_context>(&context,
                                           [](asio::io_context*) noexcept {});
}

std::error_code MakeNotOpenError() {
  return std::make_error_code(std::errc::bad_file_descriptor);
}

}  // namespace

UdpSocket::UdpSocket(Protocol protocol)
    : io_context_(std::make_shared<asio::io_context>()),
      socket_(std::make_unique<asio::ip::udp::socket>(*io_context_)) {
  open(protocol);
}

UdpSocket::UdpSocket(asio::io_context& external_context, Protocol protocol)
    : io_context_(WrapExternalContext(external_context)),
      socket_(std::make_unique<asio::ip::udp::socket>(external_context)) {
  open(protocol);
}

UdpSocket::~UdpSocket() { close(); }

std::error_code UdpSocket::open(Protocol protocol) {
  if (!socket_) {
    socket_ = std::make_unique<asio::ip::udp::socket>(*io_context_);
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

UdpSendResult UdpSocket::send(const void* data, std::size_t size) {
  if (!socket_) return {0, MakeNotOpenError()};
  UdpSendResult result{};
  asio::error_code ec;
  result.bytes_transferred = socket_->send(asio::buffer(data, size), 0, ec);
  result.error = ec;
  return result;
}

UdpSendResult UdpSocket::send_to(const void* data, std::size_t size,
                                 const Endpoint& endpoint) {
  if (!socket_) return {0, MakeNotOpenError()};
  UdpSendResult result{};
  asio::error_code ec;
  result.bytes_transferred =
      socket_->send_to(asio::buffer(data, size), endpoint.native(), 0, ec);
  result.error = ec;
  return result;
}

UdpReceiveResult UdpSocket::receive(void* data, std::size_t size) {
  if (!socket_) return {0, Endpoint(), MakeNotOpenError()};
  UdpReceiveResult result{};
  asio::error_code ec;
  result.bytes_transferred = socket_->receive(asio::buffer(data, size), 0, ec);
  result.error = ec;
  if (!ec && has_connected_endpoint_)
    result.remote_endpoint = connected_endpoint_;
  return result;
}

UdpReceiveResult UdpSocket::receive_from(void* data, std::size_t size) {
  if (!socket_) return {0, Endpoint(), MakeNotOpenError()};
  UdpReceiveResult result{};
  asio::error_code ec;
  asio::ip::udp::endpoint sender;
  result.bytes_transferred =
      socket_->receive_from(asio::buffer(data, size), sender, 0, ec);
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
