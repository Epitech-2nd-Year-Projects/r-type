#ifndef CLIENT_WORLD_UPDATE_RECEIVER_H_
#define CLIENT_WORLD_UPDATE_RECEIVER_H_

#include <atomic>
#include <cstddef>
#include <deque>
#include <memory>
#include <mutex>
#include <thread>
#include <variant>

#include "network_transport.h"
#include "protocol/command.h"
#include "protocol/message_type.h"
#include "protocol/packet.h"
#include "protocol/player_died.h"
#include "protocol/sequence_tracker.h"
#include "protocol/world_snapshot.h"

namespace client {

/**
 * @brief Gameplay update decoded from a UDP packet
 */
struct WorldUpdateMessage {
  protocol::message_type::MessageType type{};
  protocol::Header header{};
  std::variant<protocol::WorldSnapshotPayload, protocol::PlayerDiedPayload,
               protocol::CommandPayload>
      payload{};
};

/**
 * @brief Background receiver decoding gameplay packets into a queue
 */
class WorldUpdateReceiver {
 public:
  WorldUpdateReceiver() = default;
  ~WorldUpdateReceiver();

  WorldUpdateReceiver(const WorldUpdateReceiver&) = delete;
  WorldUpdateReceiver& operator=(const WorldUpdateReceiver&) = delete;
  WorldUpdateReceiver(WorldUpdateReceiver&&) noexcept = delete;
  WorldUpdateReceiver& operator=(WorldUpdateReceiver&&) noexcept = delete;

  /**
   * @brief Start the receive loop using the given transport
   */
  bool Start(NetworkTransport& transport,
             std::shared_ptr<protocol::SequenceTracker> sequence_tracker);

  /**
   * @brief Stop the receive loop and clear queued messages
   */
  void Stop();

  /**
   * @brief Pop the next queued gameplay update if available
   */
  bool TryPop(WorldUpdateMessage& out_message);

  /**
   * @brief Running state helper
   */
  bool running() const { return running_.load(std::memory_order_acquire); }

 private:
  void ReceiveLoop();
  bool ShouldQueue(protocol::message_type::MessageType type) const;
  bool Push(WorldUpdateMessage&& message);

  static constexpr std::size_t kMaxQueueDepth = 256;

  NetworkTransport* transport_{nullptr};
  std::shared_ptr<protocol::SequenceTracker> sequence_tracker_{};
  std::thread worker_;
  std::atomic<bool> running_{false};

  std::mutex queue_mutex_;
  std::deque<WorldUpdateMessage> queue_;
};

}  // namespace client

#endif  // CLIENT_WORLD_UPDATE_RECEIVER_H_
