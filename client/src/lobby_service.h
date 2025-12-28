/**
 * @file lobby_service
 * @brief Lobby service for room discovery and creation
 */

#ifndef CLIENT_LOBBY_SERVICE_H_
#define CLIENT_LOBBY_SERVICE_H_

#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "network_transport.h"
#include "protocol/lobby.h"
#include "protocol/message_type.h"
#include "protocol/packet.h"
#include "protocol/sequence_tracker.h"

namespace client {

/**
 * @brief Retry policy for lobby requests
 */
struct LobbyRetryPolicy {
  int max_attempts{4};
  std::chrono::milliseconds retry_delay{std::chrono::milliseconds(400)};
};

/**
 * @brief Lobby callbacks for status and responses
 */
struct LobbyCallbacks {
  std::function<void(std::string_view)> status_changed{};
  std::function<void(const std::vector<protocol::RoomSummary>&)>
      rooms_updated{};
  std::function<void(const protocol::CreateRoomResponsePayload&)>
      room_created{};
};

/**
 * @brief Lobby service handling room list and creation
 */
class LobbyService {
 public:
  /**
   * @brief Create a lobby service
   * @param transport Shared transport for lobby requests
   * @param retry_policy Retry policy configuration
   * @param callbacks Callback bundle for lobby updates
   */
  LobbyService(std::shared_ptr<NetworkTransport> transport,
               LobbyRetryPolicy retry_policy,
               LobbyCallbacks callbacks = {});

  /**
   * @brief Connect to a lobby endpoint
   * @param host Lobby host
   * @param port Lobby port
   * @return true when the transport is running
   */
  bool Connect(std::string host, std::uint16_t port);

  /**
   * @brief Disconnect the lobby transport
   */
  void Disconnect();

  /**
   * @brief Request a room list
   */
  void RequestRoomList();

  /**
   * @brief Request room creation
   * @param room_name Room name
   * @param is_private Privacy flag
   * @param room_password Room password
   * @param max_players Maximum player count
   */
  void RequestCreateRoom(const std::string& room_name, bool is_private,
                         std::string room_password,
                         std::uint16_t max_players);

  /**
   * @brief Pump lobby network traffic
   */
  void Update();

  /**
   * @brief Access cached rooms
   */
  const std::vector<protocol::RoomSummary>& rooms() const { return rooms_; }

  /**
   * @brief Access latest status text
   */
  const std::string& status() const { return status_text_; }

  /**
   * @brief Consume the latest room creation response
   */
  std::optional<protocol::CreateRoomResponsePayload> ConsumeCreateResponse();

 private:
  enum class Operation { kNone, kList, kCreate };

  void SendPacket(protocol::message_type::MessageType type,
                  protocol::PacketPayload payload);
  void HandlePacket(protocol::Packet& packet);
  void HandleListResponse(const protocol::RoomListResponsePayload& payload);
  void HandleCreateRoomResponse(
      const protocol::CreateRoomResponsePayload& payload);
  bool EnsureTransport();
  void ResetPending();
  void SetStatus(std::string message);
  void NotifyRoomsUpdated();

  std::shared_ptr<NetworkTransport> transport_;
  LobbyRetryPolicy retry_policy_{};
  LobbyCallbacks callbacks_{};
  std::string host_;
  std::uint16_t port_{0};
  protocol::SequenceTracker sequence_tracker_{};
  Operation active_operation_{Operation::kNone};
  std::optional<protocol::RoomListRequestPayload> pending_list_{};
  std::optional<protocol::CreateRoomRequestPayload> pending_create_{};
  std::vector<protocol::RoomSummary> rooms_{};
  std::optional<protocol::CreateRoomResponsePayload> last_create_response_{};
  std::chrono::steady_clock::time_point last_send_{};
  int attempts_{0};
  std::string status_text_{"Lobby idle"};
};

}  // namespace client

#endif  // CLIENT_LOBBY_SERVICE_H_
