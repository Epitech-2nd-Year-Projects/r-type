#include "../../include/engine/net/client.h"

#include <array>
#include <chrono>
#include <system_error>

#include <asio.hpp>

namespace engine::net {

namespace {

constexpr std::size_t kMaxDatagramSize = 2048;

bool IsTransientError(const std::error_code& ec) {
  return ec == asio::error::would_block || ec == asio::error::try_again ||
         ec == asio::error::interrupted;
}

}  // namespace

Client::Client(UdpSocket::Protocol protocol) : socket_(protocol) {}

Client::~Client() { Stop(); }

std::error_code Client::Start(const Endpoint& server_endpoint,
                              std::optional<Endpoint> bind_endpoint) {
  if (!server_endpoint.valid()) return std::make_error_code(std::errc::invalid_argument);

  Stop();

  {
    std::lock_guard<std::mutex> send_lock(send_mutex_);
    send_queue_.clear();
  }
  {
    std::lock_guard<std::mutex> recv_lock(recv_mutex_);
    recv_queue_.clear();
  }

  const auto protocol = server_endpoint.is_ipv6()
                            ? UdpSocket::Protocol::kIpv6
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

  server_endpoint_ = server_endpoint;
  running_.store(true, std::memory_order_release);
  worker_ = std::thread(&Client::WorkerLoop, this);
  return {};
}

void Client::Stop() {
  if (!running_.load(std::memory_order_acquire)) return;

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

void Client::WorkerLoop() {
  std::array<std::uint8_t, kMaxDatagramSize> recv_buffer{};

  while (running_.load(std::memory_order_acquire)) {
    PacketBuffer::Storage outgoing_bytes;
    if (DequeueOutgoing(outgoing_bytes)) {
      const auto send_result = socket_.send_to(
          outgoing_bytes.data(), outgoing_bytes.size(), server_endpoint_);
      if (send_result.error && !IsTransientError(send_result.error)) {
        running_.store(false, std::memory_order_release);
        break;
      }
    }

    const auto recv_result =
        socket_.receive_from(recv_buffer.data(), recv_buffer.size());
    if (!recv_result.error && recv_result.bytes_transferred > 0) {
      PacketBuffer packet(recv_buffer.data(), recv_result.bytes_transferred);
      ReceivedPacket received{recv_result.remote_endpoint, std::move(packet)};
      {
        std::lock_guard<std::mutex> lock(recv_mutex_);
        recv_queue_.push_back(std::move(received));
      }
      continue;
    }

    if (recv_result.error && !IsTransientError(recv_result.error)) {
      running_.store(false, std::memory_order_release);
      break;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
}

}  // namespace engine::net
