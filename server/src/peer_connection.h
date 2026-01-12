#ifndef SERVER_PEER_CONNECTION_H_
#define SERVER_PEER_CONNECTION_H_

#include <cstdint>
#include <memory>
#include <string>

#include "engine/net/endpoint.h"
#include "protocol/latency_estimator.h"
#include "protocol/reliable_queue.h"
#include "protocol/sequence_tracker.h"

namespace server {

/**
 * @brief Connection state for a remote peer.
 */
enum class PeerState : std::uint8_t {
  kConnecting = 0,   ///< Peer sent something but is not fully joined yet.
  kJoined,           ///< Peer successfully joined a game / lobby.
  kDisconnected      ///< Peer has been disconnected / timed out.
};

/**
 * @brief Per-client connection state on the server side.
 * 
 * Aggregates both transport-level information (endpoint, sequence tracking,
 * reliability, latency estimation) and gameplay-level identification (player_id, state).
 * 
 * Each connected client has one PeerConnection instance that manages:
 * - Network addressing and routing
 * - Packet sequencing and acknowledgments
 * - Reliable message delivery with retransmission
 * - Round-trip time and clock synchronization
 * - Connection lifecycle and timeout detection
 */
struct PeerConnection {
  std::string endpoint_key;          ///< Stable "ip:port" key for lookups and identification.
  engine::net::Endpoint endpoint;    ///< Remote UDP endpoint for sending packets.

  std::uint32_t player_id{0};        ///< Assigned player ID (0 = not assigned yet).
  std::uint32_t room_id{0};          ///< Assigned room identifier for the current session.
  std::string room_code;             ///< Room code chosen during join.
  std::string player_name;           ///< Player display name for chat identification.
  PeerState state{PeerState::kConnecting};  ///< Current connection state.

  std::uint32_t last_seen_ms{0};     ///< Timestamp of last received valid packet (for timeout detection).
  std::uint32_t last_acked_snapshot_id{0}; ///< Last snapshot ID acknowledged by the client.

  /**
   * @brief Protocol infrastructure helpers.
   * 
   * These components handle the low-level protocol mechanics:
   * - sequence_tracker: Manages packet sequencing and ack/ack_bits
   * - reliable_queue: Tracks reliable packets awaiting acknowledgment
   * - latency_estimator: Measures RTT and estimates clock offset
   */
  protocol::SequenceTracker sequence_tracker;              ///< Tracks local/remote sequence numbers and acks.
  std::unique_ptr<protocol::ReliableQueue> reliable_queue; ///< Queue for reliable message retransmission.
  protocol::LatencyEstimator latency_estimator;            ///< Estimates RTT and clock offset via ping/pong.
};

}  // namespace server

#endif  // SERVER_PEER_CONNECTION_H_
