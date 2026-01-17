#ifndef RIFT_CLIENT_NETWORK_SESSION_H_
#define RIFT_CLIENT_NETWORK_SESSION_H_

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

#include "ecs/world_state_system.h"
#include "engine/time/time_delta.h"
#include "join_flow.h"
#include "protocol/command.h"
#include "rift_config.h"
#include "rift_state.h"
#include "world_update_receiver.h"

namespace engine::ecs {
class Registry;
}  // namespace engine::ecs

namespace rift::client {

class NetworkTransport;

struct NetworkEvents {
  bool connected{false};
  bool stop_requested{false};
  std::optional<std::string> connection_failed;
  std::optional<std::string> disconnected;
  std::optional<MatchOverStats> match_over;
  bool opponent_joined{false};
  bool match_started{false};
};

class NetworkSession {
 public:
  explicit NetworkSession(RiftConfig config);
  ~NetworkSession();

  NetworkSession(const NetworkSession&) = delete;
  NetworkSession& operator=(const NetworkSession&) = delete;
  NetworkSession(NetworkSession&&) = delete;
  NetworkSession& operator=(NetworkSession&&) = delete;

  NetworkEvents Update(engine::time::TimeDelta dt);

  std::optional<std::string> StartConnection();

  void Stop();

  void Reset();

  void Shutdown();

  std::string_view JoinStatus() const;

  std::optional<std::uint32_t> LocalPlayerId() const;

  JoinState join_state() const { return join_flow_.state(); }

  bool TransportRunning() const;

  bool EnqueueCommand(const protocol::CommandPayload& payload);

  engine::ecs::Registry& World();
  const engine::ecs::Registry& World() const;

  std::optional<float> LatestLatencyMs() const;

  WorldUpdateReceiver& UpdateReceiver();

  std::uint32_t RoundTimerMs() const { return round_timer_ms_; }

 private:
  void HandleServerCommand(const protocol::CommandPayload& payload,
                           NetworkEvents& events);
  std::optional<std::string> HandleConnectionLost(std::string_view reason);
  void MonitorConnection(NetworkEvents& events);

  enum class DisconnectMode { kNotifyServer, kSilent };
  void Disconnect(DisconnectMode mode);

  RiftConfig config_;
  std::shared_ptr<NetworkTransport> transport_;
  JoinFlow join_flow_;
  std::unique_ptr<engine::ecs::Registry> world_registry_;
  std::unique_ptr<ecs::WorldStateSystem> world_state_system_;
  WorldUpdateReceiver world_update_receiver_;
  JoinState last_join_state_{JoinState::kIdle};
  bool disconnect_notice_sent_{false};
  std::uint32_t round_timer_ms_{0};
};

}  // namespace rift::client

#endif  // RIFT_CLIENT_NETWORK_SESSION_H_
