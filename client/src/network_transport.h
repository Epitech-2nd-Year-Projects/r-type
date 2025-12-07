#ifndef CLIENT_NETWORK_TRANSPORT_H_
#define CLIENT_NETWORK_TRANSPORT_H_

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

#include "engine/net/client.h"
#include "engine/net/endpoint.h"
#include "engine/net/packet_buffer.h"

namespace client {

/**
 * @brief Client side UDP transport facade
 *
 * @details
 * Resolves server endpoints, owns the engine::net::Client instance and exposes
 * simple start stop and queue helpers for gameplay systems
 */
class NetworkTransport {
 public:
  NetworkTransport() = default;
  ~NetworkTransport() = default;

  NetworkTransport(const NetworkTransport&) = delete;
  NetworkTransport& operator=(const NetworkTransport&) = delete;
  NetworkTransport(NetworkTransport&&) noexcept = delete;
  NetworkTransport& operator=(NetworkTransport&&) noexcept = delete;

  /**
   * @brief Start transport connected to target host and port
   * @return Error code describing failure reason
   */
  std::error_code Start(std::string_view host, std::uint16_t port,
                        std::optional<std::uint16_t> local_port = std::nullopt);

  /**
   * @brief Stop transport and clear queues
   */
  void Stop();

  /**
   * @brief Enqueue packet for send
   */
  bool Send(engine::net::PacketBuffer packet);

  /**
   * @brief Try to consume received packet
   */
  bool Receive(engine::net::Client::ReceivedPacket& out_packet);

  /**
   * @brief Access current server endpoint
   */
  engine::net::Endpoint server_endpoint() const {
    return client_.server_endpoint();
  }

  /**
   * @brief Running state helper
   */
  bool running() const { return client_.running(); }

 private:
  engine::net::Client client_{};
};

}  // namespace client

#endif  // CLIENT_NETWORK_TRANSPORT_H_
