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

bool WorldUpdateReceiver::Start(
    NetworkTransport& transport,
    std::shared_ptr<protocol::SequenceTracker> sequence_tracker) {
  if (running_.load(std::memory_order_acquire)) {
    return false;
  }
  if (!transport.running()) {
    return false;
  }

  Stop();
  transport_ = &transport;
  sequence_tracker_ = std::move(sequence_tracker);
  running_.store(true, std::memory_order_release);
  worker_ = std::thread(&WorldUpdateReceiver::ReceiveLoop, this);
  return true;
}

void WorldUpdateReceiver::Stop() {
  running_.store(false, std::memory_order_release);
  if (worker_.joinable()) {
    worker_.join();
  }

  transport_ = nullptr;
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

void WorldUpdateReceiver::ReceiveLoop() {
  engine::net::Client::ReceivedPacket incoming;

  while (running_.load(std::memory_order_acquire)) {
    bool did_work = false;
    while (running_.load(std::memory_order_acquire) && transport_ &&
           transport_->Receive(incoming)) {
      did_work = true;
      protocol::Packet packet;
      protocol::DecodeError error{protocol::DecodeError::kOk};
      if (!protocol::DecodePacket(incoming.buffer, packet, error)) {
        LogPacketError("recv decode", protocol::DecodeErrorToString(error));
        continue;
      }

      if (sequence_tracker_) {
        sequence_tracker_->OnRemoteSequenceReceived(packet.header.sequence);
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
}

}  // namespace client
