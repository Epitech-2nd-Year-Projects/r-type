#ifndef ENGINE_NET_ENDPOINT_H_
#define ENGINE_NET_ENDPOINT_H_

#ifndef ASIO_STANDALONE
#define ASIO_STANDALONE
#endif

#include <asio.hpp>
#include <cstdint>
#include <string>
#include <string_view>
#include <system_error>

namespace engine::net {

/**
 * @brief Lightweight wrapper around asio::ip::udp::endpoint
 *
 * Provides helpers to construct endpoints from strings, wildcard addresses or
 * resolver results while exposing the native ASIO endpoint when needed.
 */
class Endpoint {
 public:
  Endpoint();

  /**
   * @brief Build an endpoint bound to any IPv4 address
   */
  static Endpoint AnyIpv4(std::uint16_t port);

  /**
   * @brief Build an endpoint bound to any IPv6 address
   */
  static Endpoint AnyIpv6(std::uint16_t port);

  /**
   * @brief Build an endpoint bound to loopback IPv4
   */
  static Endpoint LoopbackIpv4(std::uint16_t port);

  /**
   * @brief Build an endpoint bound to loopback IPv6
   */
  static Endpoint LoopbackIpv6(std::uint16_t port);

  /**
   * @brief Parse numeric address string
   * @param address IPv4 or IPv6 textual address
   * @param port Network port
   * @param ec Populated when parsing fails
   */
  static Endpoint FromAddress(std::string_view address, std::uint16_t port,
                              std::error_code& ec);

  /**
   * @brief Resolve host/port pair using ASIO resolver
   *
   * The resolver runs synchronously on an internal io_context, making this
   * helper convenient for quick lookups without external plumbing.
   */
  static Endpoint Resolve(std::string_view host, std::uint16_t port,
                          std::error_code& ec);

  /**
   * @brief True when endpoint holds a valid address/port pair
   */
  bool valid() const { return valid_; }

  /**
   * @brief Human readable address string
   */
  std::string address() const;

  /**
   * @brief Port value
   */
  std::uint16_t port() const;

  /**
   * @brief Underlying ASIO endpoint
   */
  const asio::ip::udp::endpoint& native() const { return endpoint_; }

  /**
   * @brief IPv4 helper
   */
  bool is_ipv4() const { return endpoint_.address().is_v4(); }

  /**
   * @brief IPv6 helper
   */
  bool is_ipv6() const { return endpoint_.address().is_v6(); }

 private:
  friend class UdpSocket;

  explicit Endpoint(asio::ip::udp::endpoint endpoint, bool valid);

  asio::ip::udp::endpoint endpoint_;
  bool valid_;
};

}  // namespace engine::net

#endif  // ENGINE_NET_ENDPOINT_H_
