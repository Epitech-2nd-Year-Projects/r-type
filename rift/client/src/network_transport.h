#ifndef RIFT_CLIENT_NETWORK_TRANSPORT_H_
#define RIFT_CLIENT_NETWORK_TRANSPORT_H_

#include <atomic>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

#include "engine/net/client.h"
#include "engine/net/endpoint.h"
#include "engine/net/packet_buffer.h"

namespace rift::client {

class NetworkTransport {
 public:
  NetworkTransport() = default;
  ~NetworkTransport() = default;

  NetworkTransport(const NetworkTransport&) = delete;
  NetworkTransport& operator=(const NetworkTransport&) = delete;
  NetworkTransport(NetworkTransport&&) noexcept = delete;
  NetworkTransport& operator=(NetworkTransport&&) noexcept = delete;

  std::error_code Start(std::string_view host, std::uint16_t port,
                        std::optional<std::uint16_t> local_port = std::nullopt);

  void Stop();

  bool Send(engine::net::PacketBuffer packet);

  bool Receive(engine::net::Client::ReceivedPacket& out_packet);

  engine::net::Endpoint server_endpoint() const {
    return client_.server_endpoint();
  }

  bool running() const { return client_.running(); }

  std::uint64_t last_receive_ms() const {
    return last_receive_ms_.load(std::memory_order_acquire);
  }

 private:
  engine::net::Client client_{};
  std::atomic<std::uint64_t> last_receive_ms_{0};
};

}  // namespace rift::client

#endif  // RIFT_CLIENT_NETWORK_TRANSPORT_H_
