#ifndef CLIENT_JOIN_FLOW_H_
#define CLIENT_JOIN_FLOW_H_

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

namespace client {

/**
 * @brief Connection states for the join workflow
 *
 * connecting while waiting for a server reply
 * connected once a player id is assigned
 * refused when the server denies the request or times out
 * disconnected when a previously connected session is lost
 */
enum class JoinState { kIdle, kConnecting, kConnected, kRefused, kDisconnected };

/**
 * @brief Drives the JoinGame handshake and tracks the assigned player id
 *
 * Issues join requests with retries, consumes JoinAck or rejection payloads and
 * exposes human readable status text for the UI
 */
class JoinFlow {
 public:
  JoinFlow(std::string player_name, std::string room_code);
  void SetRoomPassword(std::string room_password) {
    room_password_ = std::move(room_password);
  }

  /**
   * @brief Begin the join handshake with the configured player identity
   */
  void Begin(NetworkTransport& transport);

  /**
   * @brief Configure retry policy for join attempts
   * @param max_attempts Max attempts before giving up
   * @param retry_delay Delay between join attempts
   */
  void ConfigureRetryPolicy(int max_attempts,
                            std::chrono::milliseconds retry_delay);

  /**
   * @brief Progress handshake state and consume inbound packets
   */
  void Update(NetworkTransport& transport);

  /**
   * @brief Current handshake state
   */
  JoinState state() const { return state_; }

  /**
   * @brief Assigned player identifier when connected
   */
  std::optional<std::uint32_t> player_id() const { return player_id_; }

  /**
   * @brief Latest status message suitable for HUD display
   */
  const std::string& status() const { return status_text_; }

  /**
   * @brief Access the last rejection payload if present
   */
  const std::optional<protocol::JoinRejectPayload>& rejection() const {
    return last_reject_;
  }

  /**
   * @brief Mark the current session as disconnected with a reason
   */
  void MarkDisconnected(std::string_view reason);

  /**
   * @brief Clear session state and return to idle ready for a new connection
   */
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

}  // namespace client

#endif  // CLIENT_JOIN_FLOW_H_
