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
#include "server_config.h"
#include "peer_connection.h"

namespace server {

/**
 * @brief Owning loop driving the dedicated server process
 */
class ServerRuntime {
 public:
  /**
   * @brief Construct runtime with user supplied configuration
   */
  explicit ServerRuntime(ServerConfig config);

  /**
   * @brief Open networking and configure supporting systems
   */
  std::error_code Start();

  /**
   * @brief Enter the dedicated server loop
   */
  void Run();

 private:
  void ConfigureLogging();
  void TickRateSleep(const engine::time::TimeDelta& delta_time);
  void HandlePacket(engine::net::PacketBuffer packet,
                    const engine::net::Endpoint& from);
  void ProcessJoin(PeerConnection& peer,
                   const protocol::JoinRequestPayload& request);
  void SendAccept(PeerConnection& peer);
  void SendReject(PeerConnection& peer,
                  protocol::JoinRejectReason reason,
                  std::string_view message);
  void SendPacket(PeerConnection& peer, const protocol::Packet& packet);
  PeerConnection* FindPeer(const engine::net::Endpoint& from);
  PeerConnection& GetOrCreatePeer(const engine::net::Endpoint& from);
  std::size_t CountJoinedPlayers() const;
  void CheckPeerTimeouts();

  engine::net::UdpSocket socket_;
  ServerConfig config_;
  engine::util::Logger* logger_{nullptr};
  engine::time::FrameTimer frame_timer_;
  std::uint32_t next_player_id_{1};
  std::unordered_map<std::string, PeerConnection> peers_;
  std::mt19937 rng_;
};

}  // namespace server

#endif  // SERVER_SERVER_RUNTIME_H_
