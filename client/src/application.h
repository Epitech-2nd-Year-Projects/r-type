#ifndef CLIENT_APPLICATION_H_
#define CLIENT_APPLICATION_H_

#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

#include "audio_manager.h"
#include "client_config.h"
#include "ecs/interpolation_system.h"
#include "ecs/render_system.h"
#include "ecs/world_state_system.h"
#include "engine/ecs/registry.h"
#include "engine/core/engine_runtime.h"
#include "engine/time/time_delta.h"
#include "input_layer.h"
#include "input_sender.h"
#include "join_flow.h"
#include "key_bindings.h"
#include "network_transport.h"
#include "local_prediction.h"
#include "scene/scene.h"
#include "sound_effects.h"
#include "world_update_receiver.h"
#include "debug_overlay.h"

namespace client {

/**
 * @brief High level client state controlling active screen
 *
 * Represents the coarse grained client lifecycle and drives which scene is
 * active at any given time.
 */
enum class ClientState {
  kMainMenu,
  kSettings,
  kConnecting,
  kInGame,
  kPaused,
  kGameOver,
  kDisconnected
};

/**
 * @brief High-level application object driving the client runtime
 */
class Application {
 public:
  /**
   * @brief Construct an Application with user provided configuration
   */
  explicit Application(ClientConfig config);

  int Run();

  /**
   * @brief Access the engine runtime
   */
  engine::core::EngineRuntime& GetEngine() { return *engine_; }

  /**
   * @brief Switch the active scene
   */
  void SwitchScene(std::unique_ptr<Scene> scene);

  /**
   * @brief Current high level client state
   */
  ClientState state() const { return state_; }

  /**
   * @brief Update connection configuration.
   */
  void SetConnectionConfig(std::string host, int port, std::string player_name);

  /**
   * @brief Begin the connection handshake
   */
  bool StartConnection();
  void OnConnected();
  void OnConnectionFailed(const std::string& reason);
  void OnGameStart();
  void OnGamePause();
  void OnGameResume();
  void OnGameOver();
  void OnDisconnect(std::string reason);
  void OnQuitToMenu();
  void OnOpenSettings();

  JoinFlow& GetJoinFlow() { return join_flow_; }
  NetworkTransport& GetTransport() { return *transport_; }
  WorldUpdateReceiver& GetWorldUpdateReceiver() { return world_update_receiver_; }
  /**
   * @brief Latest measured latency in milliseconds
   */
  std::optional<float> LatestLatencyMs() const;

  /**
   * @brief Current key binding configuration
   */
  const KeyBindings& key_bindings() const { return key_bindings_; }

  /**
   * @brief Replace a single action binding and persist it
   */
  bool UpdateKeyBinding(GameAction action, engine::input::Key key);

  /**
   * @brief Access the mutable ECS world
   * @note Not thread-safe; call from the main/game thread
   */
  engine::ecs::Registry& World() { return *world_registry_; }
  /**
   * @brief Access the immutable ECS world
   * @note Not thread-safe; call from the main/game thread
   */
  const engine::ecs::Registry& World() const { return *world_registry_; }
  /**
   * @brief Access the world state synchronization system
   * @note Not thread-safe; call from the main/game thread
   */
  ecs::WorldStateSystem& WorldSync() { return *world_state_system_; }

 private:
  bool Tick(engine::time::TimeDelta dt);
  void UpdateAudio(engine::time::TimeDelta dt, JoinState join_state);
  void HandleGameOverAudio();
  void HandleServerCommand(const protocol::CommandPayload& payload);
  void MonitorConnection(JoinState join_state);
  void HandleConnectionLost(std::string_view reason);
  void HandleReconnectInput(JoinState join_state);
  void ProcessJoinState(JoinState join_state);
  void ApplyState(ClientState next_state, std::string reason = {});
  bool TransitionTo(ClientState next_state, std::string reason = {});
  bool IsTransitionAllowed(ClientState next_state) const;
  void StopNetworkSession();
  void LoadKeyBindings();
  bool SaveKeyBindings();
  void UpdateDebugOverlayState();
  std::size_t RenderableEntityCount() const;
  void CommitSceneChange();

  ClientConfig config_;
  std::shared_ptr<NetworkTransport> transport_;
  JoinFlow join_flow_;
  std::unique_ptr<engine::core::EngineRuntime> engine_;
  std::unique_ptr<engine::ecs::Registry> world_registry_;
  std::unique_ptr<ecs::WorldStateSystem> world_state_system_;
  std::unique_ptr<ecs::InterpolationSystem> interpolation_system_;
  std::unique_ptr<ecs::RenderSystem> render_system_;
  std::unique_ptr<InputLayer> input_layer_;
  std::unique_ptr<InputSender> input_sender_;
  std::unique_ptr<LocalPrediction> local_prediction_;
  std::unique_ptr<AudioManager> audio_manager_;
  std::unique_ptr<SoundEffects> sound_effects_;
  std::unique_ptr<Scene> current_scene_;
  std::unique_ptr<Scene> pending_scene_;
  WorldUpdateReceiver world_update_receiver_;
  KeyBindings key_bindings_{KeyBindings::Default()};
  std::filesystem::path keybindings_path_{"config/keybindings.json"};
  ClientState state_{ClientState::kMainMenu};
  std::string disconnect_reason_;
  JoinState last_join_state_{JoinState::kIdle};
  bool music_allowed_{false};
  bool music_blocked_{false};
  bool reconnect_requested_{false};
  bool debug_toggle_pressed_{false};
  DebugOverlay debug_overlay_{};
};

}  // namespace client

#endif  // CLIENT_APPLICATION_H_
