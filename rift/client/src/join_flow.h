#ifndef RIFT_CLIENT_JOIN_FLOW_H_
#define RIFT_CLIENT_JOIN_FLOW_H_

#include <chrono>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

#include "network_transport.h"
#include "protocol/join.h"
#include "protocol/packet.h"
#include "protocol/sequence_tracker.h"

namespace rift::client {

enum class JoinState { kIdle, kConnecting, kConnected, kRefused, kDisconnected };

class JoinFlow {
 public:
  JoinFlow(std::string player_name, std::string room_code);
  void SetRoomPassword(std::string room_password) {
    room_password_ = std::move(room_password);
  }

  void Begin(NetworkTransport& transport);

  void ConfigureRetryPolicy(int max_attempts,
                            std::chrono::milliseconds retry_delay);

  void Update(NetworkTransport& transport);

  JoinState state() const { return state_; }

  std::optional<std::uint32_t> player_id() const { return player_id_; }

  const std::string& status() const { return status_text_; }

  const std::optional<protocol::JoinRejectPayload>& rejection() const {
    return last_reject_;
  }

  void MarkDisconnected(std::string_view reason);

  void Reset();

 private:
  void SendJoinRequest(NetworkTransport& transport);
  void HandleDecodedPacket(protocol::Packet& packet);
  void HandleJoinAccept(const protocol::JoinAcceptPayload& payload);
  void HandleJoinReject(const protocol::JoinRejectPayload& payload);
  void Fail(std::string_view message);

  static constexpr int kDefaultMaxAttempts = 5;
  static constexpr std::chrono::milliseconds kDefaultRetryDelay{500};

  JoinState state_{JoinState::kIdle};
  std::string player_name_;
  std::string room_code_;
  std::string room_password_;
  std::optional<std::uint32_t> player_id_;
  std::optional<protocol::JoinRejectPayload> last_reject_;
  std::string status_text_;
  std::uint32_t next_sequence_{1};
  int attempts_{0};
  std::chrono::steady_clock::time_point last_send_{};
  int max_attempts_{kDefaultMaxAttempts};
  std::chrono::milliseconds retry_delay_{kDefaultRetryDelay};
  protocol::SequenceTracker sequence_tracker_{};
};

}  // namespace rift::client

#endif  // RIFT_CLIENT_JOIN_FLOW_H_
