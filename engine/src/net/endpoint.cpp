#include "../../include/engine/net/endpoint.h"

#include <sstream>
#include <utility>

namespace engine::net {

Endpoint::Endpoint() : endpoint_(), valid_(false) {}

Endpoint::Endpoint(asio::ip::udp::endpoint endpoint, bool valid)
    : endpoint_(std::move(endpoint)), valid_(valid) {}

Endpoint Endpoint::AnyIpv4(std::uint16_t port) {
  return Endpoint(asio::ip::udp::endpoint(asio::ip::udp::v4(), port), true);
}

Endpoint Endpoint::AnyIpv6(std::uint16_t port) {
  return Endpoint(asio::ip::udp::endpoint(asio::ip::udp::v6(), port), true);
}

Endpoint Endpoint::LoopbackIpv4(std::uint16_t port) {
  return Endpoint(
      asio::ip::udp::endpoint(asio::ip::address_v4::loopback(), port), true);
}

Endpoint Endpoint::LoopbackIpv6(std::uint16_t port) {
  return Endpoint(
      asio::ip::udp::endpoint(asio::ip::address_v6::loopback(), port), true);
}

Endpoint Endpoint::FromAddress(std::string_view address, std::uint16_t port,
                               std::error_code& ec) {
  auto addr = asio::ip::make_address(address, ec);
  if (ec) return Endpoint();
  return Endpoint(asio::ip::udp::endpoint(addr, port), true);
}

Endpoint Endpoint::Resolve(std::string_view host, std::uint16_t port,
                           std::error_code& ec) {
  asio::io_context io;
  asio::ip::udp::resolver resolver(io);
  auto service = std::to_string(port);
  auto results = resolver.resolve(host, service, ec);
  if (ec) return Endpoint();
  if (results.empty()) {
    ec = asio::error::not_found;
    return Endpoint();
  }
  return Endpoint(*results.begin(), true);
}

std::string Endpoint::address() const {
  if (!valid_) return {};
  std::stringstream ss;
  ss << endpoint_.address().to_string() << ":" << endpoint_.port();
  return ss.str();
}

std::uint16_t Endpoint::port() const {
  if (!valid_) return 0;
  return endpoint_.port();
}

}  // namespace engine::net
