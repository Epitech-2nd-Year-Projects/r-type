#include "world_update_receiver.h"

#include <chrono>
#include <optional>
#include <utility>

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

}  // namespace

WorldUpdateReceiver::~WorldUpdateReceiver() { Stop(); }

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
  if (worker_.joinable()) {
    worker_.join();
  }

  transport_.reset();
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

void WorldUpdateReceiver::ReceiveLoop() {
  engine::net::Client::ReceivedPacket incoming;

  while (running_.load(std::memory_order_acquire)) {
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
}

}  // namespace client
