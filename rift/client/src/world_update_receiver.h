#ifndef RIFT_CLIENT_WORLD_UPDATE_RECEIVER_H_
#define RIFT_CLIENT_WORLD_UPDATE_RECEIVER_H_

#include <atomic>
#include <chrono>
#include <cstddef>
#include <condition_variable>
#include <deque>
#include <memory>
#include <mutex>
#include <optional>
#include <thread>
#include <variant>

#include "network_transport.h"
#include "protocol/command.h"
#include "protocol/input_state.h"
#include "protocol/latency_estimator.h"
#include "protocol/message_type.h"
#include "protocol/packet.h"
#include "protocol/ping.h"
#include "protocol/player_died.h"
#include "protocol/sequence_tracker.h"
#include "protocol/world_snapshot.h"
#include "protocol/snapshot_history.h"
#include "protocol/fragmentation.h"

namespace rift::client {

struct OutgoingMessage {
  protocol::message_type::MessageType type{
      protocol::message_type::MessageType::kInvalid};
  protocol::InputStatePayload input_state{};
  protocol::CommandPayload command_payload{};
  std::uint32_t client_time_ms{0};
};

struct WorldUpdateMessage {
  protocol::message_type::MessageType type{};
  protocol::Header header{};
  std::variant<protocol::WorldSnapshotPayload, protocol::PlayerDiedPayload,
               protocol::CommandPayload>
      payload{};
};

class WorldUpdateReceiver {
 public:
  WorldUpdateReceiver() = default;
  ~WorldUpdateReceiver();

  WorldUpdateReceiver(const WorldUpdateReceiver&) = delete;
  WorldUpdateReceiver& operator=(const WorldUpdateReceiver&) = delete;
  WorldUpdateReceiver(WorldUpdateReceiver&&) noexcept = delete;
  WorldUpdateReceiver& operator=(WorldUpdateReceiver&&) noexcept = delete;

  bool Start(std::shared_ptr<NetworkTransport> transport);

  void Configure(std::chrono::milliseconds ping_interval,
                 std::size_t queue_depth);

  void Stop();

  bool TryPop(WorldUpdateMessage& out_message);

  bool EnqueueInputState(const protocol::InputStatePayload& payload,
                         std::uint32_t client_time_ms);

  bool EnqueueCommand(const protocol::CommandPayload& payload);

  bool running() const { return running_.load(std::memory_order_acquire); }

  std::optional<float> LatestRttMs() const;

  std::uint32_t LatestSnapshotId() const;

 private:
  void ReceiveLoop();
  bool Push(WorldUpdateMessage&& message);
  bool SendPing(std::uint32_t client_time_ms);
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
  std::atomic<std::uint64_t> last_pong_ms_{0};
  protocol::SnapshotHistory snapshot_history_{32};
  std::atomic<std::uint32_t> last_received_snapshot_id_{0};
  protocol::FragmentReassembler reassembler_;
};

}  // namespace rift::client

#endif  // RIFT_CLIENT_WORLD_UPDATE_RECEIVER_H_
