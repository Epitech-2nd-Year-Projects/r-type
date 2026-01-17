#ifndef CLIENT_WORLD_UPDATE_RECEIVER_H_
#define CLIENT_WORLD_UPDATE_RECEIVER_H_
/**
 * @file world_update_receiver.h
 * @brief Background decoding of gameplay updates
 */

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <deque>
#include <memory>
#include <mutex>
#include <optional>
#include <thread>
#include <variant>

#include "network_transport.h"
#include "protocol/command.h"
#include "protocol/fragmentation.h"
#include "protocol/input_state.h"
#include "protocol/latency_estimator.h"
#include "protocol/message_type.h"
#include "protocol/packet.h"
#include "protocol/ping.h"
#include "protocol/player_died.h"
#include "protocol/sequence_tracker.h"
#include "protocol/snapshot_history.h"
#include "protocol/world_snapshot.h"

class WorldUpdateReceiverTestPeer;

namespace client {

/**
 * @brief Outgoing message from gameplay to the network thread
 */
struct OutgoingMessage {
  protocol::message_type::MessageType type{
      protocol::message_type::MessageType::kInvalid};  ///< Message type to
                                                       ///< send.
  protocol::InputStatePayload input_state{};  ///< Populated for kInputState.
  protocol::CommandPayload
      command_payload{};            ///< Populated for kClientCommand.
  std::uint32_t client_time_ms{0};  ///< Client timestamp for inputs.
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
 * for consumption on the main thread. It also owns a bounded outgoing queue
 * for gameplay payloads (inputs) that need headers and encoding before send.
 * Start is synchronous and will stop any previous worker before launching a
 * new one.
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
   * @param transport Shared transport used for send/receive ownership is shared
   * @return true when the worker thread is launched, false if the transport is
   * not running or the receiver is already active
   */
  bool Start(std::shared_ptr<NetworkTransport> transport);

  /**
   * @brief Configure ping interval and queue depth
   * @param ping_interval Interval between ping packets
   * @param queue_depth Maximum queued message count
   */
  void Configure(std::chrono::milliseconds ping_interval,
                 std::size_t queue_depth);

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
   * @brief Enqueue an input state payload to be encoded and sent on the network
   * thread
   * @param payload Input payload to encode and send
   * @param client_time_ms Client timestamp for the packet header
   * @return true when queued, false if the worker is not running or queue is
   * full
   */
  bool EnqueueInputState(const protocol::InputStatePayload& payload,
                         std::uint32_t client_time_ms);

  /**
   * @brief Enqueue a command payload to be encoded and sent on the network
   * thread
   * @param payload Command payload to encode and send
   * @return true when queued, false if the worker is not running or queue is
   * full
   */
  bool EnqueueCommand(const protocol::CommandPayload& payload);

  /**
   * @brief Running state helper
   * @return true when the background worker is active
   */
  bool running() const { return running_.load(std::memory_order_acquire); }

  /**
   * @brief Latest measured round-trip latency in milliseconds
   * @return Populated when at least one ping/pong cycle completed
   */
  std::optional<float> LatestRttMs() const;

  /**
   * @brief Latest estimated clock offset (server - client) in milliseconds
   */
  std::optional<float> LatestClockOffsetMs() const;

  /**
   * @brief Returns the ID of the last successfully received and processed
   * snapshot
   */
  std::uint32_t LatestSnapshotId() const;

 private:
  friend class ::WorldUpdateReceiverTestPeer;
  void ReceiveLoop();
  bool Push(WorldUpdateMessage&& message);
  /**
   * @brief Send a ping packet stamped with the provided client time.
   * @param client_time_ms Local timestamp to embed in the ping header.
   * @return true when the ping was queued for send.
   */
  bool SendPing(std::uint32_t client_time_ms);
  /**
   * @brief Update latency estimation based on a received pong.
   * @param pong Payload echoed from the server.
   * @param now_ms Local receipt time used to compute RTT.
   */
  void HandlePong(const protocol::PongPayload& pong, std::uint32_t now_ms);

  std::shared_ptr<NetworkTransport> transport_{};
  protocol::SequenceTracker sequence_tracker_{};
  std::condition_variable outgoing_cv_;
  std::mutex outgoing_mutex_;
  std::deque<OutgoingMessage> outgoing_queue_;
  std::thread worker_;
  std::atomic<bool> running_{false};

  std::mutex queue_mutex_;
  std::deque<WorldUpdateMessage> queue_;

  std::size_t max_queue_depth_{256};
  std::chrono::milliseconds ping_interval_{1000};
  protocol::LatencyEstimator latency_estimator_{};
  std::atomic<bool> has_latency_estimate_{false};
  std::atomic<float> latest_rtt_ms_{0.0f};
  std::atomic<float> latest_clock_offset_ms_{0.0f};
  std::atomic<std::uint64_t> last_pong_ms_{0};
  protocol::SnapshotHistory snapshot_history_{32};

  std::atomic<std::uint32_t> last_received_snapshot_id_{0};
  protocol::FragmentReassembler reassembler_;
};

}  // namespace client

#endif  // CLIENT_WORLD_UPDATE_RECEIVER_H_
