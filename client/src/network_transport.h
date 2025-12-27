#ifndef CLIENT_NETWORK_TRANSPORT_H_
#define CLIENT_NETWORK_TRANSPORT_H_

#include <atomic>
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
#ifdef RTYPE_TESTING
    if (test_state_.has_value()) {
      return test_state_->server_endpoint;
    }
#endif
    return client_.server_endpoint();
  }

  /**
   * @brief Running state helper
   */
  bool running() const {
#ifdef RTYPE_TESTING
    if (test_state_.has_value()) {
      return test_state_->running;
    }
#endif
    return client_.running();
  }

  /**
   * @brief Timestamp in milliseconds of the last received packet
   */
  std::uint64_t last_receive_ms() const {
    return last_receive_ms_.load(std::memory_order_acquire);
  }

 private:
#ifdef RTYPE_TESTING
  struct TestState {
    engine::net::Endpoint server_endpoint{};
    bool running{false};
  };
#endif

  engine::net::Client client_{};
#ifdef RTYPE_TESTING
  std::optional<TestState> test_state_{};
#endif
  std::atomic<std::uint64_t> last_receive_ms_{0};
};

}  // namespace client

#endif  // CLIENT_NETWORK_TRANSPORT_H_
