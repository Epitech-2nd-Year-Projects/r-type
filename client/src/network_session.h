/**
 * @file network_session.h
 * @brief Network session management for the client
 */

#ifndef CLIENT_NETWORK_SESSION_H_
#define CLIENT_NETWORK_SESSION_H_

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "client_config.h"
#include "client_state.h"
#include "engine/time/time_delta.h"
#include "join_flow.h"
#include "protocol/command.h"
#include "protocol/lobby.h"
#include "world_update_receiver.h"
#include "protocol/gameplay_ping.h"

namespace engine::ecs {
class Registry;
}  // namespace engine::ecs

namespace engine::debug {
class NetworkDebugger;
}

namespace client {

class AudioController;
class NetworkTransport;
class LobbyService;

namespace ecs {
class AnimationSystem;
class InterpolationSystem;
class WorldStateSystem;
}  // namespace ecs

/**
 * @brief Network update notifications for the application
 */
struct NetworkEvents {
  bool connected{false};
  bool stop_requested{false};
  std::optional<std::string> connection_failed;
  std::optional<std::string> disconnected;
  std::optional<GameOverStats> game_over;
  std::vector<std::string> chat_messages;  ///< Incoming chat messages.
  std::vector<protocol::GameplayPingPayload> gameplay_pings; ///< Incoming gameplay pings.
};

/**
 * @brief Network session owning transports and join flow
 */
class NetworkSession {
 public:
  /**
   * @brief Construct a network session
   * @param config Initial client config
   */
  explicit NetworkSession(ClientConfig config);

  /**
   * @brief Destroy the network session
   */
  ~NetworkSession();

  NetworkSession(const NetworkSession&) = delete;
  NetworkSession& operator=(const NetworkSession&) = delete;
  NetworkSession(NetworkSession&&) = delete;
  NetworkSession& operator=(NetworkSession&&) = delete;

  /**
   * @brief Update network state
   * @param dt Frame delta
   * @param audio Audio controller
   * @return Network events
   */
  NetworkEvents Update(engine::time::TimeDelta dt, AudioController& audio);

  /**
   * @brief Start a connection to the configured host
   * @return Optional error message
   */
  std::optional<std::string> StartConnection();

  /**
   * @brief Stop active network systems
   */
  void Stop();

  /**
   * @brief Reset join flow and cached state
   */
  void Reset();

  /**
   * @brief Shutdown network resources
   */
  void Shutdown();

  /**
   * @brief Attach a network debugger
   */
  void AttachDebugger(engine::debug::NetworkDebugger& debugger);

  /**
   * @brief Update connection configuration
   */
  void SetConnectionConfig(std::string host, int port, std::string player_name,
                           std::string room_code,
                           std::string room_password = {});

  /**
   * @brief Request a fresh room list
   */
  void RefreshRoomList(std::string host, std::uint16_t port);

  /**
   * @brief Ask the server to create a room
   */
  void CreateRoom(std::string host, std::uint16_t port, std::string room_name,
                  bool is_private, std::string room_password,
                  std::uint16_t max_players, protocol::Difficulty difficulty);

  /**
   * @brief Access room summaries
   */
  const std::vector<protocol::RoomSummary>& RoomDirectoryRooms() const;

  /**
   * @brief Room directory status text
   */
  std::string RoomDirectoryStatus() const;

  /**
   * @brief Latest room creation response
   */
  std::optional<protocol::CreateRoomResponsePayload> ConsumeLastRoomCreation();

  /**
   * @brief Access join status message
   */
  std::string_view JoinStatus() const;

  /**
   * @brief Access the local player id
   */
  std::optional<std::uint32_t> LocalPlayerId() const;

  /**
   * @brief Access join state
   */
  JoinState join_state() const { return join_flow_.state(); }

  /**
   * @brief Access transport running state
   */
  bool TransportRunning() const;

  /**
   * @brief Enqueue a server command
   */
  bool EnqueueCommand(const protocol::CommandPayload& payload);

  /**
   * @brief Enqueue a gameplay ping
   */
  bool EnqueueGameplayPing(const protocol::GameplayPingPayload& payload);

  /**
   * @brief Access the ECS registry
   */
  engine::ecs::Registry& World();

  /**
   * @brief Access the ECS registry read only view
   */
  const engine::ecs::Registry& World() const;

  /**
   * @brief Latest wave number
   */
  std::optional<std::uint32_t> CurrentWave() const;

  /**
   * @brief Latest latency in milliseconds
   */
  std::optional<float> LatestLatencyMs() const;

  /**
   * @brief Access the world update receiver
   */
  WorldUpdateReceiver& UpdateReceiver();

 private:
  void HandleServerCommand(const protocol::CommandPayload& payload,
                           NetworkEvents& events);
  std::optional<std::string> HandleConnectionLost(std::string_view reason);
  void MonitorConnection(NetworkEvents& events);
  void UpdateLocalPlayerCache();
  GameOverStats BuildGameOverStats() const;

  enum class DisconnectMode { kNotifyServer, kSilent };
  void Disconnect(DisconnectMode mode);

  ClientConfig config_;
  std::shared_ptr<NetworkTransport> transport_;
  std::shared_ptr<NetworkTransport> lobby_transport_;
  JoinFlow join_flow_;
  std::unique_ptr<LobbyService> lobby_service_;
  std::unique_ptr<engine::ecs::Registry> world_registry_;
  std::unique_ptr<ecs::WorldStateSystem> world_state_system_;
  std::unique_ptr<ecs::AnimationSystem> animation_system_;
  std::unique_ptr<ecs::InterpolationSystem> interpolation_system_;
  WorldUpdateReceiver world_update_receiver_;
  std::optional<std::uint32_t> cached_local_score_;
  std::optional<std::uint32_t> last_wave_{1u};
  JoinState last_join_state_{JoinState::kIdle};
  bool disconnect_notice_sent_{false};
};

}  // namespace client

#endif  // CLIENT_NETWORK_SESSION_H_
