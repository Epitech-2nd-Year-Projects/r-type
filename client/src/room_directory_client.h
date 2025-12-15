#ifndef CLIENT_ROOM_DIRECTORY_CLIENT_H_
#define CLIENT_ROOM_DIRECTORY_CLIENT_H_

#include <chrono>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "engine/util/logging.h"
#include "network_transport.h"
#include "protocol/lobby.h"
#include "protocol/message_type.h"
#include "protocol/packet.h"
#include "protocol/sequence_tracker.h"

namespace client {

/**
 * @brief Lightweight lobby client to list and create rooms before joining.
 */
class RoomDirectoryClient {
 public:
  explicit RoomDirectoryClient(std::shared_ptr<NetworkTransport> transport);

  /**
   * @brief Connect to a lobby endpoint (idempotent).
   */
  bool Connect(std::string host, std::uint16_t port);

  /**
   * @brief Stop the lobby transport.
   */
  void Disconnect();

  /**
   * @brief Request the current room directory.
   */
  void RequestRoomList();

  /**
   * @brief Request creation of a room.
   */
  void RequestCreateRoom(const std::string& desired_code,
                         bool is_private,
                         std::uint16_t max_players);

  /**
   * @brief Pump network I/O and retry pending operations.
   */
  void Update();

  const std::vector<protocol::RoomSummary>& rooms() const { return rooms_; }
  const std::string& status() const { return status_text_; }

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

  std::shared_ptr<NetworkTransport> transport_;
  std::string host_;
  std::uint16_t port_{0};
  protocol::SequenceTracker sequence_tracker_;
  Operation active_operation_{Operation::kNone};
  std::optional<protocol::RoomListRequestPayload> pending_list_;
  std::optional<protocol::CreateRoomRequestPayload> pending_create_;
  std::vector<protocol::RoomSummary> rooms_;
  std::optional<protocol::CreateRoomResponsePayload> last_create_response_;
  std::chrono::steady_clock::time_point last_send_{};
  int attempts_{0};
  std::string status_text_{"Lobby idle"};
  engine::util::Logger& logger_;
};

}  // namespace client

#endif  // CLIENT_ROOM_DIRECTORY_CLIENT_H_
