#ifndef SERVER_SERVER_RUNTIME_H_
#define SERVER_SERVER_RUNTIME_H_

#include <cstdint>
#include <random>
#include <string>
#include <string_view>
#include <system_error>
#include <unordered_map>

#include "engine/net/udp_socket.h"
#include "engine/net/packet_buffer.h"
#include "engine/time/frame_timer.h"
#include "engine/util/logging.h"
#include "protocol/join.h"
#include "protocol/packet.h"
#include "protocol/input_state.h"
#include "protocol/command.h"
#include "server_config.h"
#include "peer_connection.h"
#include "game_instance.h"

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
   * @brief Sends a join accept message to a peer.
   * @param peer The peer connection to accept.
   * 
   * Constructs and sends a JoinAcceptPayload with server configuration
   * and assigned player ID.
   */
  void SendAccept(PeerConnection& peer);
  
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
   * @brief Finds an existing peer connection by endpoint.
   * @param from The endpoint to search for.
   * @return Pointer to the peer connection if found, nullptr otherwise.
   */
  PeerConnection* FindPeer(const engine::net::Endpoint& from);

  /**
   * @brief Finds an existing peer connection by player ID.
   * @param player_id The player ID to search for.
   * @return Pointer to the peer connection if found, nullptr otherwise.
   */
  PeerConnection* FindPeerByPlayerId(std::uint32_t player_id);

  /**
   * @brief Removes a peer connection by endpoint key.
   * @param peer The peer connection to remove.
   */
  void RemovePeer(PeerConnection& peer);
  
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
   * @brief Counts the number of peers in the kJoined state.
   * @return Number of fully joined players.
   */
  std::size_t CountJoinedPlayers() const;
  
  /**
   * @brief Checks all peer connections for inactivity timeouts.
   * 
   * Disconnects peers that haven't sent a valid packet within the
   * configured timeout period. Called periodically during the main loop.
   */
  void CheckPeerTimeouts();

  /**
   * @brief Sends the current game state to all connected players.
   * 
   * Constructs a WorldSnapshotPayload containing entity deltas and broadcasts
   * it to all peers in the kJoined state. Increments next_snapshot_id_ after
   * sending to maintain snapshot ordering.
   */
  void BroadcastWorldSnapshot();

  engine::net::UdpSocket socket_;                           ///< UDP socket for network communication.
  ServerConfig config_;                                     ///< Server configuration (port, tick rate, limits, etc.).
  engine::util::Logger* logger_{nullptr};                  ///< Logger instance for diagnostic output.
  engine::time::FrameTimer frame_timer_;                    ///< Timer for maintaining fixed tick rate.
  std::uint32_t next_player_id_{1};                        ///< Next available player ID for assignment.
  std::unordered_map<std::string, PeerConnection> peers_;  ///< Map of endpoint keys to peer connections.
  std::unordered_map<std::uint32_t, std::string> players_;      ///< Map of player IDs to endpoint keys.
  std::mt19937 rng_;                                        ///< Random number generator for deterministic seeds.
  GameInstance game_instance_;                              ///< Authoritative game instance.
  std::uint32_t server_tick_{0};                            ///< Current server tick counter since startup.
  std::uint32_t next_snapshot_id_{1};                       ///< Next snapshot ID for world state broadcasts.
  engine::time::TimeDelta fixed_delta_;                     ///< Fixed simulation timestep (1.0 / tick_rate).
  engine::time::TimeDelta accumulator_;                     ///< Accumulates frame time for fixed-step simulation.
  bool running_{false};                                     ///< Whether the server loop is currently running.
};

}  // namespace server

#endif  // SERVER_SERVER_RUNTIME_H_
