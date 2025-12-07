#ifndef ENGINE_NET_CLIENT_H_
#define ENGINE_NET_CLIENT_H_

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <deque>
#include <mutex>
#include <optional>
#include <system_error>
#include <thread>

#include "endpoint.h"
#include "packet_buffer.h"
#include "udp_socket.h"

namespace engine::net {

/**
 * @brief UDP client with managed lifecycle
 *
 * @details
 * Maintains a single UDP socket connected to a remote endpoint with optional
 * local binding
 * Provides thread safe queues for outgoing and incoming datagrams and a worker
 * loop to drive non blocking I/O
 */
class Client {
 public:
  /**
   * @brief Received packet bundle
   */
  struct ReceivedPacket {
    Endpoint from;
    PacketBuffer buffer;
  };

  Client();
  ~Client();

  Client(const Client&) = delete;
  Client& operator=(const Client&) = delete;
  Client(Client&&) noexcept = delete;
  Client& operator=(Client&&) noexcept = delete;

  /**
   * @brief Start client I/O loop
   *
   * @param server_endpoint Remote server endpoint
   * @param bind_endpoint Optional local bind endpoint
   * @return Error code indicating startup status
   */
  std::error_code Start(const Endpoint& server_endpoint,
                        std::optional<Endpoint> bind_endpoint = std::nullopt);

  /**
   * @brief Stop I/O loop and close socket
   */
  void Stop();

  /**
   * @brief Enqueue packet for sending
   * @return false when client not running
   */
  bool Enqueue(PacketBuffer packet);

  /**
   * @brief Pop next received packet if present
   * @return false when queue empty
   */
  bool TryDequeue(ReceivedPacket& out_packet);

  /**
   * @brief Running state helper
   */
  bool running() const { return running_.load(std::memory_order_acquire); }

  /**
   * @brief Connected server endpoint
   */
  Endpoint server_endpoint() const;

 private:
  void WorkerLoop();
  bool DequeueOutgoing(PacketBuffer::Storage& out_bytes);

  static constexpr std::size_t kMaxQueueDepth = 1024;

  UdpSocket socket_;
  Endpoint server_endpoint_;
  mutable std::mutex endpoint_mutex_;

  std::thread worker_;
  std::atomic<bool> running_{false};

  std::mutex send_mutex_;
  std::condition_variable send_cv_;
  std::deque<PacketBuffer::Storage> send_queue_;

  std::mutex recv_mutex_;
  std::deque<ReceivedPacket> recv_queue_;
};

}  // namespace engine::net

#endif  // ENGINE_NET_CLIENT_H_
