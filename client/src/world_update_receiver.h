#ifndef CLIENT_WORLD_UPDATE_RECEIVER_H_
#define CLIENT_WORLD_UPDATE_RECEIVER_H_
/**
 * @file world_update_receiver.h
 * @brief Background decoding of gameplay updates
 */

#include <atomic>
#include <cstddef>
#include <deque>
#include <memory>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <variant>

#include "network_transport.h"
#include "protocol/command.h"
#include "protocol/input_state.h"
#include "protocol/message_type.h"
#include "protocol/packet.h"
#include "protocol/player_died.h"
#include "protocol/sequence_tracker.h"
#include "protocol/world_snapshot.h"

namespace client {

/**
 * @brief Outgoing message from gameplay to the network thread
 */
struct OutgoingMessage {
  protocol::message_type::MessageType type{
      protocol::message_type::MessageType::kInvalid};  ///< Message type to send.
  protocol::InputStatePayload input_state{};           ///< Populated for kInputState.
  std::uint32_t client_time_ms{0};                     ///< Client timestamp for inputs.
};

/**
 * @brief Gameplay update decoded from a UDP packet
 */
struct WorldUpdateMessage {
  protocol::message_type::MessageType type{};  ///< Message type for routing.
  protocol::Header header{};  ///< Packet header for sequencing and timing.
  std::variant<protocol::WorldSnapshotPayload, protocol::PlayerDiedPayload,
               protocol::CommandPayload>
      payload{};  ///< Decoded payload moved from the received packet.
};

/**
 * @class WorldUpdateReceiver
 * @brief Background receiver decoding gameplay packets into a queue
 *
 * @details
 * Spawns a worker thread that drains the transport receive queue, decodes
 * protocol packets and stores relevant gameplay messages in a bounded queue
 * for consumption on the main thread. Start is synchronous and will stop any
 * previous worker before launching a new one.
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
   * @param transport Non-owning transport reference that must outlive this
   * receiver
   * @return true when the worker thread is launched, false if the transport is
   * not running or the receiver is already active
   */
  bool Start(NetworkTransport& transport);

  /**
   * @brief Stop the receive loop and clear queued messages
   */
  void Stop();

  /**
   * @brief Pop the next queued gameplay update if available
   * @param out_message Destination for the dequeued message
   * @return true when a message was written, false when the queue is empty
   */
  bool TryPop(WorldUpdateMessage& out_message);

  /**
   * @brief Enqueue an input state payload to be encoded and sent on the network thread
   */
  bool EnqueueInputState(const protocol::InputStatePayload& payload,
                         std::uint32_t client_time_ms);

  /**
   * @brief Running state helper
   * @return true when the background worker is active
   */
  bool running() const { return running_.load(std::memory_order_acquire); }

 private:
  void ReceiveLoop();
  bool Push(WorldUpdateMessage&& message);

  static constexpr std::size_t kMaxQueueDepth = 256;

  NetworkTransport* transport_{nullptr};
  protocol::SequenceTracker sequence_tracker_{};
  std::condition_variable outgoing_cv_;
  std::mutex outgoing_mutex_;
  std::deque<OutgoingMessage> outgoing_queue_;
  std::thread worker_;
  std::atomic<bool> running_{false};

  std::mutex queue_mutex_;
  std::deque<WorldUpdateMessage> queue_;
};

}  // namespace client

#endif  // CLIENT_WORLD_UPDATE_RECEIVER_H_
