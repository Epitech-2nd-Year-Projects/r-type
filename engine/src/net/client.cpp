#include "../../include/engine/net/client.h"

#include <array>
#include <asio.hpp>
#include <chrono>
#include <span>
#include <system_error>

namespace engine::net {

namespace {

constexpr std::size_t kMaxDatagramSize = 2048;

bool IsTransientError(const std::error_code& ec) {
  return ec == asio::error::would_block || ec == asio::error::try_again ||
         ec == asio::error::interrupted;
}

}  // namespace

Client::Client() : socket_(UdpSocket::Protocol::kIpv4) {}

Client::~Client() { Stop(); }

std::error_code Client::Start(const Endpoint& server_endpoint,
                              std::optional<Endpoint> bind_endpoint) {
  if (!server_endpoint.valid())
    return std::make_error_code(std::errc::invalid_argument);

  Stop();

  {
    std::lock_guard<std::mutex> send_lock(send_mutex_);
    send_queue_.clear();
  }
  {
    std::lock_guard<std::mutex> recv_lock(recv_mutex_);
    recv_queue_.clear();
  }

  const auto protocol = server_endpoint.is_ipv6() ? UdpSocket::Protocol::kIpv6
                                                  : UdpSocket::Protocol::kIpv4;
  if (auto open_error = socket_.open(protocol); open_error) {
    return open_error;
  }

  if (bind_endpoint.has_value()) {
    if (auto bind_error = socket_.bind(*bind_endpoint); bind_error) {
      return bind_error;
    }
  }

  if (auto connect_error = socket_.connect(server_endpoint); connect_error) {
    return connect_error;
  }

  {
    std::lock_guard<std::mutex> lock(endpoint_mutex_);
    server_endpoint_ = server_endpoint;
  }
  running_.store(true, std::memory_order_release);
  worker_ = std::thread(&Client::WorkerLoop, this);
  return {};
}

void Client::Stop() {
  running_.store(false, std::memory_order_release);
  send_cv_.notify_all();
  if (worker_.joinable()) worker_.join();

  socket_.close();

  {
    std::lock_guard<std::mutex> send_lock(send_mutex_);
    send_queue_.clear();
  }
  {
    std::lock_guard<std::mutex> recv_lock(recv_mutex_);
    recv_queue_.clear();
  }
}

bool Client::Enqueue(PacketBuffer packet) {
  if (!running_.load(std::memory_order_acquire)) return false;
  std::lock_guard<std::mutex> lock(send_mutex_);
  if (send_queue_.size() >= kMaxQueueDepth) return false;
  send_queue_.push_back(packet.storage());
  send_cv_.notify_one();
  return true;
}

bool Client::TryDequeue(ReceivedPacket& out_packet) {
  std::lock_guard<std::mutex> lock(recv_mutex_);
  if (recv_queue_.empty()) return false;
  out_packet = std::move(recv_queue_.front());
  recv_queue_.pop_front();
  return true;
}

bool Client::DequeueOutgoing(PacketBuffer::Storage& out_bytes) {
  std::unique_lock<std::mutex> lock(send_mutex_);
  send_cv_.wait_for(lock, std::chrono::milliseconds(1), [this] {
    return !send_queue_.empty() || !running_.load(std::memory_order_acquire);
  });
  if (!running_.load(std::memory_order_acquire)) return false;
  if (send_queue_.empty()) return false;
  out_bytes = std::move(send_queue_.front());
  send_queue_.pop_front();
  return true;
}

Endpoint Client::server_endpoint() const {
  std::lock_guard<std::mutex> lock(endpoint_mutex_);
  return server_endpoint_;
}

void Client::WorkerLoop() {
  std::array<std::uint8_t, kMaxDatagramSize> recv_buffer{};

  while (running_.load(std::memory_order_acquire)) {
    bool did_work = false;
    Endpoint endpoint_copy;
    {
      std::lock_guard<std::mutex> lock(endpoint_mutex_);
      endpoint_copy = server_endpoint_;
    }

    PacketBuffer::Storage outgoing_bytes;
    if (DequeueOutgoing(outgoing_bytes)) {
      did_work = true;
      const auto send_result =
          socket_.send_to(std::span<const std::uint8_t>(outgoing_bytes),
                          endpoint_copy);
      if (send_result.error && !IsTransientError(send_result.error)) {
        running_.store(false, std::memory_order_release);
        break;
      }
    }

    const auto recv_result = socket_.receive_from(std::span(recv_buffer));
    if (!recv_result.error && recv_result.bytes_transferred > 0) {
      did_work = true;
      const auto packet_view =
          std::span(recv_buffer).first(recv_result.bytes_transferred);
      PacketBuffer packet(packet_view);
      ReceivedPacket received{recv_result.remote_endpoint, std::move(packet)};
      {
        std::lock_guard<std::mutex> lock(recv_mutex_);
        if (recv_queue_.size() < kMaxQueueDepth) {
          recv_queue_.push_back(std::move(received));
        }
      }
    }

    if (recv_result.error && !IsTransientError(recv_result.error)) {
      running_.store(false, std::memory_order_release);
      break;
    }

    if (!did_work) {
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    } else {
      std::this_thread::yield();
    }
  }
}

}  // namespace engine::net
