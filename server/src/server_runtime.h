#ifndef SERVER_SERVER_RUNTIME_H_
#define SERVER_SERVER_RUNTIME_H_

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <random>
#include <string>
#include <string_view>
#include <thread>
#include <system_error>
#include <unordered_map>

#include <asio/thread_pool.hpp>

#include "engine/net/packet_buffer.h"
#include "engine/time/frame_timer.h"
#include "engine/util/logging.h"
#include "protocol/join.h"
#include "protocol/packet.h"
#include "protocol/input_state.h"
#include "protocol/command.h"
#include "protocol/error.h"
#include "protocol/fragmentation.h"
#include "protocol/lobby.h"
#include "server_config.h"
#include "peer_connection.h"
#include "room.h"
#include "server_transport.h"

namespace server {

/**
 * @brief Main runtime loop managing the dedicated R-Type server process.
 * 
 * ServerRuntime orchestrates all server-side operations:
 * - Network packet reception and processing
 * - Client connection and session management
 * - Fixed-rate game loop execution
 * - Protocol message handling and routing
 * 
 * The server operates with a fixed tick rate, processing incoming packets,
 * updating game state, and sending updates to connected clients.
 */
class ServerRuntime {
 public:
  /**
   * @brief Constructs the server runtime with user-supplied configuration.
   * @param config Server configuration (port, tick rate, max players, etc.).
   */
  explicit ServerRuntime(ServerConfig config);

  /**
   * @brief Initializes networking and configures supporting systems.
   * @return Error code indicating success or failure reason.
   * 
   * Opens the UDP socket, binds to the configured port, and initializes
   * logging and timing subsystems. Must be called before Run().
   */
  std::error_code Start();

  /**
   * @brief Enters the main server loop.
   * 
   * Runs the fixed-rate game loop, processing packets, updating game state,
   * and sending updates to clients. Blocks until the server is shut down.
   * 
   * @note Requires Start() to have been called successfully first.
   */
 void Run();

  /**
   * @brief Executes one iteration of the main server loop.
   * 
   * Processes network packets, updates game simulation, checks timeouts,
   * and broadcasts world snapshots. Called repeatedly by Run() at the
   * configured tick rate using a fixed timestep accumulator.
  */
  void RunMainLoop();

 private:
  static constexpr std::uint32_t kReliableResendTimeoutMs = 250;
  static constexpr std::size_t kReliableQueueMaxPending = 64;
  static constexpr std::uint32_t kDecodeMetricsLogIntervalMs = 10'000;
  static constexpr std::uint32_t kServerDiagnosticsLogIntervalMs = 10'000;
  static constexpr float kTickHealthWarningThreshold = 0.9f;

  /**
   * @brief Updates all rooms, dispatching work across the thread pool.
   *
   * Each room is updated exactly once per tick. A latch ensures the main
   * thread waits for all room updates to finish before proceeding to
   * snapshot broadcast / network send.
   */
  void UpdateRoomsParallel(const engine::time::TimeDelta& delta);

  /**
   * @brief Configures the logging system based on server configuration.
   * 
   * Sets the global log level according to config.log_level and initializes
   * the logger instance for server diagnostic output.
   */
  void ConfigureLogging();

  /**
   * @brief Polls the UDP socket for incoming packets.
   * 
   * Non-blocking read of all available packets from the socket.
   * Each received packet is dispatched to HandlePacket for processing.
   */
  void PollNetwork();
  
  /**
   * @brief Sleeps to maintain the configured tick rate.
   * @param delta_time Time elapsed since last frame.
   * 
   * Ensures the server runs at a fixed tick rate by sleeping for the
   * remaining time after processing the current frame.
   */
  void TickRateSleep(const engine::time::TimeDelta& delta_time);

  /**
   * @brief Periodically logs decode metrics when interval elapsed or forced.
   * 
   * Checks elapsed time or a force flag and emits a decode metrics summary log
   * when appropriate to aid monitoring.
   */
  void MaybeLogDecodeMetrics();

  /**
   * @brief Periodically logs server-level diagnostics.
   *
   * Emits counts for peers and rooms alongside tickrate health indicators to
   * surface degraded performance.
   */
  void MaybeLogServerStats();

  /**
   * @brief Logs the current sessions, peers, and rooms.
   */
  void DumpSessions();

  /**
   * @brief Reloads runtime configuration such as log level.
   */
  void ReloadConfiguration();
  /**
   * @brief Logs a summary of decode metrics.
   * @param force If true logs metrics regardless of packet count.
   * 
   * Emits aggregated decode statistics for diagnostics and performance
   * monitoring.
   */
  void LogDecodeMetricsSummary(bool force);


  /**
   * @brief Sends a server command message to a specific peer.
   * @param peer The destination peer connection.
   * @param command_id The command identifier.
   * @param payload The command payload data.
   * 
   * Constructs a CommandPayload and sends it to the specified peer.
   * Used for server-initiated commands like game state changes,
   * administrative actions, or event notifications.
   */
  void SendServerCommand(PeerConnection& peer,
                         std::uint16_t command_id,
                         std::string_view payload);

  /**
   * @brief Broadcasts a server command message to all joined peers.
   * @param command_id The command identifier.
   * @param payload The command payload data.
   * 
   * Sends a CommandPayload to all peers in the kJoined state.
   * Used for game-wide events, announcements, or synchronized
   * state changes affecting all players.
   */
  void BroadcastServerCommand(std::uint16_t command_id,
                              std::string_view payload);

  /**
   * @brief Handles an incoming packet from a remote endpoint.
   * @param packet The received packet buffer.
   * @param from The remote endpoint that sent the packet.
   * 
   * Decodes the packet, routes it to the appropriate handler based on
   * message type, and updates peer connection state.
   */
  void HandlePacket(engine::net::PacketBuffer packet,
                    const engine::net::Endpoint& from);

  /**
   * @brief Handles an incoming ping message from a peer.
   * @param peer The peer connection that sent the ping.
   * @param ping The ping payload containing client timestamp.
   *
   * Updates the peer's last activity timestamp and may respond with a pong.
   */
  void HandlePing(PeerConnection& peer, const protocol::PingPayload& ping);

  /**
   * @brief Handles an incoming input state message from a peer.
   * @param peer The peer connection that sent the input state.
   * @param input_state The input state payload containing player inputs.
   * @param header The packet header associated with the input state.
   *
   * Processes the player's input commands and updates game state accordingly.
   */
  void HandleInputState(PeerConnection& peer,
                          const protocol::InputStatePayload& input_state,
                          const protocol::Header& header);

  /**
   * @brief Handles an incoming client command message from a peer.
   * @param peer The peer connection that sent the command.
   * @param command The command payload containing command ID and data.
   * @param header The packet header associated with the command.
   *
   * Processes the client command and updates game state accordingly.
   */
  void HandleClientCommand(PeerConnection& peer,
                           const protocol::CommandPayload& command,
                           const protocol::Header& header);
  
  /**
   * @brief Processes a join request from a peer.
   * @param peer The peer connection requesting to join.
   * @param request The join request payload.
   * 
   * Validates the request, assigns a player ID if successful, and sends
   * either an accept or reject response.
   */
  void ProcessJoin(PeerConnection& peer,
                   const protocol::JoinRequestPayload& request);
  /**
   * @brief Handles a request for the current room directory.
   *
   * Sends the peer a list of public rooms ordered by name, capped to the
   * protocol limit.
   */
  void HandleRoomListRequest(PeerConnection& peer);
  /**
   * @brief Handles creation of a new room.
   */
  void HandleCreateRoomRequest(
      PeerConnection& peer,
      const protocol::CreateRoomRequestPayload& request);
  
  /**
   * @brief Sends a join accept message to a peer.
   * @param peer The peer connection to accept.
   * @param room_code Room code associated with the peer.
   * 
   * Constructs and sends a JoinAcceptPayload with server configuration
   * and assigned player ID.
   */
  void SendAccept(PeerConnection& peer, const std::string& room_code);
  
  /**
   * @brief Sends a join reject message to a peer.
   * @param peer The peer connection to reject.
   * @param reason The reason code for rejection.
   * @param message Human-readable explanation for the rejection.
   * 
   * Constructs and sends a JoinRejectPayload with the specified reason.
   */
  void SendReject(PeerConnection& peer,
                  protocol::JoinRejectReason reason,
                  std::string_view message);
  
  /**
   * @brief Sends a packet to a specific peer.
   * @param peer The destination peer connection.
   * @param packet The packet to send.
   * 
   * Encodes the packet and sends it via UDP to the peer's endpoint.
   */
  void SendPacket(PeerConnection& peer, const protocol::Packet& packet);

  /**
   * @brief Applies ACK information from the remote header to the reliable queue.
   * @param peer The peer connection whose reliable queue to update.
   * @param header The incoming packet header containing ack/ack_bits.
   * 
   * Marks any pending reliable packets as acknowledged based on the incoming
   * ack/ack_bits fields. Removes acknowledged packets from the retransmission
   * queue to prevent unnecessary resends.
   */
  void ProcessPeerAcks(PeerConnection& peer, const protocol::Header& header);

  /**
   * @brief Retransmits timed-out reliable packets for all peers.
   * 
   * Periodically called from the main loop to keep reliable messages flowing.
   * Checks each peer's reliable queue for packets that have timed out and
   * resends them to ensure guaranteed delivery of critical messages.
   */
  void ProcessReliableResends();
  
  /**
   * @brief Finds an existing peer connection by endpoint.
   * @param from The endpoint to search for.
   * @return Peer connection reference if found.
   */
  std::optional<std::reference_wrapper<PeerConnection>> FindPeer(
      const engine::net::Endpoint& from);

  /**
   * @brief Finds an existing peer connection by player ID.
   * @param player_id The player ID to search for.
   * @return Peer connection reference if found.
   */
  std::optional<std::reference_wrapper<PeerConnection>> FindPeerByPlayerId(
      std::uint32_t player_id);

  /**
   * @brief Marks a peer as disconnected and cleans up player mappings.
   * @param peer Peer connection to disconnect.
   * @param reason Human-readable reason for logging and notifications.
   * @param notify_client Whether to attempt sending a disconnect notice.
   *
   * @note This function does not erase the peer from peers_. Callers are
   *       responsible for removing the entry after invoking this helper.
   */
  void DisconnectPeer(PeerConnection& peer,
                      std::string_view reason,
                      bool notify_client);

  /**
   * @brief Generates a room summary suitable for lobby responses.
   */
  protocol::RoomSummary BuildRoomSummary(const Room& room) const;

  /**
   * @brief Ensures a default public room exists for quick joins.
   */
  void EnsureDefaultRoom();

  /**
   * @brief Removes a peer connection by endpoint key.
   * @param peer The peer connection to remove.
   */
  void RemovePeer(PeerConnection& peer);

  /**
   * @brief Joins a peer to a room and updates bookkeeping.
   * @param peer Peer to attach.
   * @param room_code Target room code.
   * @param player_name Player display name.
   * @return true if join succeeded.
   */
  bool JoinRoom(PeerConnection& peer, Room& room, std::string_view player_name);

  /**
   * @brief Removes a peer from its room and updates bookkeeping.
   * @param peer Peer to detach.
   * @param now_ms Current timestamp for activity tracking.
   * @return Resolved room code the peer left (empty if none).
   */
  std::string LeaveRoom(PeerConnection& peer, std::uint32_t now_ms);
  
  /**
   * @brief Gets or creates a peer connection for an endpoint.
   * @param from The endpoint to find or create.
   * @return Reference to the peer connection (existing or newly created).
   * 
   * If no peer exists for the endpoint, creates a new one in the
   * kConnecting state.
   */
  PeerConnection& GetOrCreatePeer(const engine::net::Endpoint& from);

  /**
   * @brief Checks all peer connections for inactivity timeouts.
   * 
   * Disconnects peers that haven't sent a valid packet within the
   * configured timeout period. Called periodically during the main loop.
   */
  void CheckPeerTimeouts();
  /**
   * @brief Removes player sessions that lost their peer connection.
   *
   * Safeguards against lingering player counts when a peer vanishes without
   * sending a formal disconnect.
   */
  void PruneOrphanedSessions();

  /**
   * @brief Sends the current game state to connected players per room.
   * 
   * Constructs a WorldSnapshotPayload for each active room and broadcasts
   * it to peers that joined the corresponding room.
   */
  void BroadcastWorldSnapshots();

  /**
   * @brief Broadcasts non-snapshot game events (e.g. death)
   */
  void BroadcastGameEvents();

  /**
   * @brief Tracks session details for a connected player.
   *
   * Associates a player identifier with the endpoint they came from and
   * the room they joined to support room-aware routing.
   */
  struct PlayerSession {
    std::string endpoint_key; ///< Key used to locate the peer connection.
    std::string room_code;    ///< Room that the player is currently part of.
  };

  /**
  * @brief Finds a room by its code.
  * @param room_code Room code to search for.
   * @return Reference to the room if present.
   */
  std::optional<std::reference_wrapper<Room>> FindRoom(
      const std::string& room_code);

  /**
   * @brief Finds a const room by its code.
   * @param room_code Room code to search for.
   * @return Const reference to the room if present.
   */
  std::optional<std::reference_wrapper<const Room>> FindRoomConst(
      const std::string& room_code) const;

  /**
   * @brief Creates a new room with the given metadata.
   * @param room_code Room code to resolve.
   * @param is_private Whether the room is private.
   * @param max_players Capacity for the room.
   * @return Reference to the created room instance.
   */
  Room& CreateRoom(const std::string& room_code,
                   const std::string& room_name,
                   bool is_private,
                   std::string password,
                   std::uint16_t max_players);

  /**
   * @brief Removes a room when no peers remain.
   * @param room_code Room code to check and remove if empty or idle.
   * @param now_ms Current timestamp for idle evaluation.
   */
  void CleanupRoomIfEmpty(const std::string& room_code, std::uint32_t now_ms);

  ServerTransport transport_;                              ///< UDP transport facade for non-blocking send/recv.
  ServerConfig config_;                                    ///< Server configuration (port, tick rate, limits, etc.).
  engine::util::Logger& logger_;                           ///< Logger instance for diagnostic output.
  engine::time::FrameTimer frame_timer_;                   ///< Timer for maintaining fixed tick rate.
  const std::size_t worker_count_{1};                      ///< Number of worker threads for room simulation.
  asio::thread_pool worker_pool_;                          ///< Pool executing per-room updates in parallel.
  std::uint32_t next_player_id_{1};                        ///< Next available player ID for assignment.
  std::unordered_map<std::string, PeerConnection> peers_;  ///< Map of endpoint keys to peer connections.
  std::unordered_map<std::uint32_t, PlayerSession> players_; ///< Map of player IDs to player sessions.
  std::mt19937 rng_;                                       ///< Random number generator for deterministic seeds.
  std::unordered_map<std::string, Room> rooms_;            ///< Active room contexts keyed by room code.
  std::uint32_t next_room_id_{1};                          ///< Counter for room identifiers.
  std::uint32_t server_tick_{0};                           ///< Current server tick counter since startup.
  engine::time::TimeDelta fixed_delta_;                    ///< Fixed simulation timestep (1.0 / tick_rate).
  engine::time::TimeDelta accumulator_;                    ///< Accumulates frame time for fixed-step simulation.
  bool running_{false};                                    ///< Whether the server loop is currently running.
  protocol::DecodeMetrics decode_metrics_{};               ///< Tracks packet decode statistics (success/error counts).
  std::uint32_t last_decode_metrics_log_ms_{0};            ///< Last timestamp when decode metrics were logged.
  std::uint32_t last_server_stats_log_ms_{0};              ///< Last timestamp when server stats were logged.
  std::uint32_t last_tick_health_sample_ms_{0};            ///< Start timestamp for the tick health window.
  std::uint32_t last_tick_health_sample_tick_{0};          ///< Tick counter at start of the health window.
  double frame_time_accumulator_ms_{0.0};                  ///< Accumulated frame durations for health averages.
#include "protocol/fragmentation.h"

// ... existing includes ...

  std::uint64_t frame_time_samples_{0};                    ///< Frame count contributing to health averages.
  protocol::FragmentReassembler reassembler_;              ///< Handles reassembly of fragmented packets.
};

}  // namespace server

#endif  // SERVER_SERVER_RUNTIME_H_
