/**
 * @file server_transport.h
 * @brief Server-side UDP transport facade built on engine::net
 */

#ifndef SERVER_SERVER_TRANSPORT_H_
#define SERVER_SERVER_TRANSPORT_H_

#include <cstddef>
#include <cstdint>
#include <span>
#include <system_error>
#include <vector>

#include "engine/net/endpoint.h"
#include "engine/net/packet_buffer.h"
#include "engine/net/udp_socket.h"

namespace server {

/**
 * @brief Lightweight wrapper around engine::net UDP primitives
 *
 * Opens a UDP socket on the configured port, collects incoming datagrams into
 * PacketBuffers and provides helpers to send raw bytes to remote endpoints.
 * The transport keeps networking concerns isolated from higher level gameplay
 * code while remaining non-blocking.
 */
class ServerTransport {
 public:
  using UdpEndpoint = engine::net::Endpoint;

  /**
   * @brief One UDP datagram received from a remote endpoint
   */
  struct Datagram {
    UdpEndpoint from;
    engine::net::PacketBuffer payload;
  };

  /**
   * @brief Result bundle returned by PollNetwork
   */
  struct PollResult {
    std::vector<Datagram> datagrams;
    std::error_code error{};
  };

  ServerTransport() = default;
  ~ServerTransport() = default;

  ServerTransport(const ServerTransport&) = delete;
  ServerTransport& operator=(const ServerTransport&) = delete;
  ServerTransport(ServerTransport&&) noexcept = delete;
  ServerTransport& operator=(ServerTransport&&) noexcept = delete;

  /**
   * @brief Open and bind the UDP socket on the given port
   * @param port UDP port to bind
   * @return Error code describing startup failure
   *
   * @details
   * Stops any existing socket, opens a fresh non-blocking UDP socket and binds
   * to the requested port. Returns the first error encountered when opening or
   * binding.
   */
  std::error_code Start(std::uint16_t port);

  /**
   * @brief Close the socket and reset state
   */
  void Stop();

  /**
   * @brief Send raw bytes to a remote endpoint
   * @param to Destination endpoint
   * @param data Pointer to payload bytes
   * @param size Number of bytes to send
   * @return Outcome of the send operation
   */
  engine::net::UdpSendResult Send(const UdpEndpoint& to,
                                  const void* data,
                                  std::size_t size);

  /**
   * @brief Send contents of a PacketBuffer to a remote endpoint
   * @param to Destination endpoint
   * @param buffer Encoded packet payload
   * @return Outcome of the send operation
   */
  engine::net::UdpSendResult Send(const UdpEndpoint& to,
                                  const engine::net::PacketBuffer& buffer);

  /**
   * @brief Drain all pending datagrams since last call
   * @return Collection of received packets and any fatal error
   *
   * @details
   * Performs a non-blocking loop receiving all available UDP datagrams and
   * returns them as PacketBuffers. On transient errors it stops polling but
   * keeps running; on fatal errors it records the error and marks the transport
   * as stopped.
   */
  PollResult PollNetwork();

  /**
   * @brief Endpoint the socket is bound to
   */
  const UdpEndpoint& local_endpoint() const { return bound_endpoint_; }

  /**
   * @brief Running state helper
   */
  bool running() const { return running_; }

 private:
  static constexpr std::size_t kMaxDatagramSize = 2048;

  engine::net::UdpSocket socket_{engine::net::UdpSocket::Protocol::kIpv4};  ///< Underlying UDP socket.
  UdpEndpoint bound_endpoint_{};                                            ///< Local endpoint the socket is bound to.
  bool running_{false};                                                     ///< Transport running state.
};

}  // namespace server

#endif  // SERVER_SERVER_TRANSPORT_H_
