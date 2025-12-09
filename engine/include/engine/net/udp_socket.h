#ifndef ENGINE_NET_UDP_SOCKET_H_
#define ENGINE_NET_UDP_SOCKET_H_

#ifndef ASIO_STANDALONE
#define ASIO_STANDALONE
#endif

#include <asio.hpp>
#include <cstdint>
#include <cstddef>
#include <functional>
#include <memory>
#include <span>
#include <system_error>

#include "endpoint.h"

namespace engine::net {

/**
 * @brief Outcome for send operations
 */
struct UdpSendResult {
  std::size_t bytes_transferred{0};
  std::error_code error{};
};

/**
 * @brief Outcome for receive operations
 */
struct UdpReceiveResult {
  std::size_t bytes_transferred{0};
  Endpoint remote_endpoint{};
  std::error_code error{};
};

/**
 * @brief Simple non-blocking UDP socket wrapper
 *
 * Uses ASIO datagram sockets underneath and exposes synchronous,
 * non-blocking helpers suitable for simple networking layers. The socket can
 * either own its io_context or attach to an external one.
 */
class UdpSocket {
 public:
  enum class Protocol { kIpv4, kIpv6 };

  explicit UdpSocket(Protocol protocol = Protocol::kIpv4);
  UdpSocket(asio::io_context& external_context,
            Protocol protocol = Protocol::kIpv4);
  ~UdpSocket();

  UdpSocket(const UdpSocket&) = delete;
  UdpSocket& operator=(const UdpSocket&) = delete;
  UdpSocket(UdpSocket&&) noexcept = default;
  UdpSocket& operator=(UdpSocket&&) noexcept = default;

  /**
   * @brief Open socket for given protocol and switch to non-blocking mode
   */
  std::error_code open(Protocol protocol);

  /**
   * @brief Bind socket to endpoint
   */
  std::error_code bind(const Endpoint& endpoint);

  /**
   * @brief Connect socket to remote endpoint
   */
  std::error_code connect(const Endpoint& endpoint);

  /**
   * @brief Send data to connected peer
   */
  UdpSendResult send(std::span<const std::uint8_t> data);

  /**
   * @brief Send data to explicit endpoint
   */
  UdpSendResult send_to(std::span<const std::uint8_t> data,
                        const Endpoint& endpoint);

  /**
   * @brief Receive from connected peer
   */
  UdpReceiveResult receive(std::span<std::uint8_t> data);

  /**
   * @brief Receive from any sender
   */
  UdpReceiveResult receive_from(std::span<std::uint8_t> data);

  /**
   * @brief Underlying socket status
   */
  bool is_open() const { return socket_ && socket_->is_open(); }

  /**
   * @brief Close socket
   */
  void close();

  /**
   * @brief Access owned io_context (useful to pump events)
   */
  asio::io_context& io_context() { return context_ref_.get(); }

 private:
  std::shared_ptr<asio::io_context> owned_context_;
  std::reference_wrapper<asio::io_context> context_ref_;
  std::unique_ptr<asio::ip::udp::socket> socket_;
  Endpoint connected_endpoint_;
  bool has_connected_endpoint_{false};
};

}  // namespace engine::net

#endif  // ENGINE_NET_UDP_SOCKET_H_
