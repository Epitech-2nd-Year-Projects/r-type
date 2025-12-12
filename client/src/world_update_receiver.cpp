#include "world_update_receiver.h"

#include <chrono>
#include <optional>
#include <utility>

#include "engine/time/monotonic_time.h"
#include "logging.h"
#include "protocol/error.h"

namespace client {

namespace {

std::optional<WorldUpdateMessage> MakeWorldUpdateMessage(
    protocol::Packet& packet) {
  const auto type = static_cast<protocol::message_type::MessageType>(
      packet.header.message_type);

  switch (type) {
    case protocol::message_type::MessageType::kWorldSnapshot: {
      WorldUpdateMessage message{};
      message.type = type;
      message.header = packet.header;
      message.payload =
          std::get<protocol::WorldSnapshotPayload>(std::move(packet.payload));
      return message;
    }
    case protocol::message_type::MessageType::kPlayerDied: {
      WorldUpdateMessage message{};
      message.type = type;
      message.header = packet.header;
      message.payload =
          std::get<protocol::PlayerDiedPayload>(std::move(packet.payload));
      return message;
    }
    case protocol::message_type::MessageType::kServerCommand: {
      WorldUpdateMessage message{};
      message.type = type;
      message.header = packet.header;
      message.payload =
          std::get<protocol::CommandPayload>(std::move(packet.payload));
      return message;
    }
    default:
      return std::nullopt;
  }
}

constexpr auto kPingInterval = std::chrono::milliseconds(1000);

std::uint32_t NowMilliseconds32() {
  return static_cast<std::uint32_t>(engine::time::NowMilliseconds());
}

}  // namespace

WorldUpdateReceiver::~WorldUpdateReceiver() { Stop(); }

std::optional<float> WorldUpdateReceiver::LatestRttMs() const {
  if (!has_latency_estimate_.load(std::memory_order_acquire)) {
    return std::nullopt;
  }
  return latest_rtt_ms_.load(std::memory_order_acquire);
}

bool WorldUpdateReceiver::Start(std::shared_ptr<NetworkTransport> transport) {
  if (running_.load(std::memory_order_acquire)) {
    return false;
  }
  if (!transport || !transport->running()) {
    return false;
  }

  Stop();
  transport_ = std::move(transport);
  sequence_tracker_.Reset();
  running_.store(true, std::memory_order_release);
  worker_ = std::thread(&WorldUpdateReceiver::ReceiveLoop, this);
  return true;
}

void WorldUpdateReceiver::Stop() {
  running_.store(false, std::memory_order_release);
  outgoing_cv_.notify_all();
  if (worker_.joinable()) {
    worker_.join();
  }
  has_latency_estimate_.store(false, std::memory_order_release);
  latest_rtt_ms_.store(0.0f, std::memory_order_release);
  last_pong_ms_.store(0, std::memory_order_release);

  transport_.reset();
  {
    std::lock_guard<std::mutex> lock(outgoing_mutex_);
    outgoing_queue_.clear();
  }
  {
    std::lock_guard<std::mutex> lock(queue_mutex_);
    queue_.clear();
  }
}

bool WorldUpdateReceiver::TryPop(WorldUpdateMessage& out_message) {
  std::lock_guard<std::mutex> lock(queue_mutex_);
  if (queue_.empty()) {
    return false;
  }
  out_message = std::move(queue_.front());
  queue_.pop_front();
  return true;
}

bool WorldUpdateReceiver::Push(WorldUpdateMessage&& message) {
  std::lock_guard<std::mutex> lock(queue_mutex_);
  if (queue_.size() >= kMaxQueueDepth) {
    return false;
  }
  queue_.push_back(std::move(message));
  return true;
}

bool WorldUpdateReceiver::EnqueueInputState(
    const protocol::InputStatePayload& payload, std::uint32_t client_time_ms) {
  if (!running_.load(std::memory_order_acquire)) {
    return false;
  }
  std::lock_guard<std::mutex> lock(outgoing_mutex_);
  if (outgoing_queue_.size() >= kMaxQueueDepth) {
    return false;
  }
  OutgoingMessage message{};
  message.type = protocol::message_type::MessageType::kInputState;
  message.input_state = payload;
  message.client_time_ms = client_time_ms;
  outgoing_queue_.push_back(std::move(message));
  outgoing_cv_.notify_one();
  return true;
}

bool WorldUpdateReceiver::SendPing(std::uint32_t client_time_ms) {
  if (!transport_ || !transport_->running()) {
    return false;
  }

  protocol::PingPayload ping{};
  ping.client_time_ms = client_time_ms;

  protocol::Packet packet{};
  packet.header.version = protocol::kProtocolVersion;
  packet.header.message_type =
      static_cast<std::uint8_t>(protocol::message_type::MessageType::kPing);
  packet.header.flags = 0;
  packet.header.sequence = sequence_tracker_.NextLocalSequence();
  packet.header.ack = 0;
  packet.header.ack_bits = 0;
  packet.header.timestamp_ms = client_time_ms;
  sequence_tracker_.FillAckFields(packet.header);
  packet.payload = ping;

  engine::net::PacketBuffer buffer;
  buffer.reserve(64);
  if (!protocol::EncodePacket(packet, buffer)) {
    return false;
  }

  const bool queued = transport_->Send(std::move(buffer));
  if (queued) {
    latency_estimator_.OnPingSent(client_time_ms);
  }
  return queued;
}

void WorldUpdateReceiver::HandlePong(const protocol::PongPayload& pong,
                                     std::uint32_t now_ms) {
  latency_estimator_.OnPongReceived(pong.client_time_ms, pong.server_time_ms,
                                    now_ms);
  has_latency_estimate_.store(latency_estimator_.has_estimate(),
                              std::memory_order_release);
  latest_rtt_ms_.store(latency_estimator_.rtt_ms(), std::memory_order_release);
  last_pong_ms_.store(now_ms, std::memory_order_release);
}

void WorldUpdateReceiver::ReceiveLoop() {
  engine::net::Client::ReceivedPacket incoming;
  bool transport_reported_stopped = false;
  auto last_ping_sent = std::chrono::steady_clock::now();
  bool has_sent_initial_ping = false;

  while (running_.load(std::memory_order_acquire)) {
    if (!transport_ || !transport_->running()) {
      if (!transport_reported_stopped) {
        LogLifecycle(engine::util::LogLevel::kWarn,
                     "Stopping receiver: transport not running");
        transport_reported_stopped = true;
      }
      running_.store(false, std::memory_order_release);
      break;
    }

    bool did_work = false;
    {
      std::unique_lock<std::mutex> lock(outgoing_mutex_);
      if (outgoing_cv_.wait_for(lock, std::chrono::milliseconds(1), [this] {
            return !outgoing_queue_.empty() ||
                   !running_.load(std::memory_order_acquire);
          })) {
        while (running_.load(std::memory_order_acquire) &&
               !outgoing_queue_.empty()) {
          did_work = true;
          OutgoingMessage message = std::move(outgoing_queue_.front());
          outgoing_queue_.pop_front();
          lock.unlock();

          protocol::Packet packet{};
          packet.header.version = protocol::kProtocolVersion;
          packet.header.message_type = static_cast<std::uint8_t>(message.type);
          packet.header.flags = 0;
          packet.header.sequence = sequence_tracker_.NextLocalSequence();
          packet.header.ack = 0;
          packet.header.ack_bits = 0;
          packet.header.timestamp_ms = message.client_time_ms;
          sequence_tracker_.FillAckFields(packet.header);
          packet.payload = message.input_state;

          engine::net::PacketBuffer buffer;
          buffer.reserve(128);
          if (protocol::EncodePacket(packet, buffer)) {
            transport_->Send(std::move(buffer));
          } else {
            LogPacketError("encode", "failed to encode outgoing payload");
          }

          lock.lock();
        }
      }
    }

    const auto now = std::chrono::steady_clock::now();
    const bool should_ping =
        !has_sent_initial_ping || now - last_ping_sent >= kPingInterval;
    if (should_ping && SendPing(NowMilliseconds32())) {
      did_work = true;
      has_sent_initial_ping = true;
      last_ping_sent = now;
    }

    while (running_.load(std::memory_order_acquire) && transport_ &&
           transport_->Receive(incoming)) {
      did_work = true;
      protocol::Packet packet;
      protocol::DecodeError error{protocol::DecodeError::kOk};
      if (!protocol::DecodePacket(incoming.buffer, packet, error)) {
        LogPacketError("recv decode", protocol::DecodeErrorToString(error));
        continue;
      }

      sequence_tracker_.OnRemoteSequenceReceived(packet.header.sequence);

      const auto type = static_cast<protocol::message_type::MessageType>(
          packet.header.message_type);
      if (type == protocol::message_type::MessageType::kPong) {
        if (std::holds_alternative<protocol::PongPayload>(packet.payload)) {
          HandlePong(std::get<protocol::PongPayload>(packet.payload),
                     NowMilliseconds32());
        }
        continue;
      }

      auto message = MakeWorldUpdateMessage(packet);
      if (!message.has_value()) {
        continue;
      }

      if (!Push(std::move(*message))) {
        LogPacketError("recv queue", "world update queue full");
      }
    }

    if (!did_work) {
      std::this_thread::sleep_for(std::chrono::milliseconds(2));
    } else {
      std::this_thread::yield();
    }
  }
  running_.store(false, std::memory_order_release);
}

}  // namespace client
