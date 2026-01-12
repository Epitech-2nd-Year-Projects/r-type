#ifndef CLIENT_APPLICATION_H_
#define CLIENT_APPLICATION_H_

#include <chrono>
#include <memory>

#include "client_config.h"
#include "client_context.h"
#include "lobby_chat_service.h"
#include "player_profile.h"
#include "engine/time/time_delta.h"

namespace client {

class AudioController;
class ClientAssetManager;
class ClientRuntime;
class InputCoordinator;
namespace ui {
class MenuBackground;
}
struct NetworkEvents;
class NetworkSession;
class SceneManager;

/**
 * @brief High level application object driving the client runtime
 */
class Application : public ClientContext {
 public:
  /**
   * @brief Construct an Application with user provided configuration
   * @param config Client configuration
   */
  explicit Application(ClientConfig config);

  /**
   * @brief Destroy the application
   */
  ~Application() override;

  /**
   * @brief Run the client loop
   * @return Exit code
   */
  int Run();

  engine::render::Renderer2D& Renderer() override;
  engine::input::InputManager& Input() override;
  const KeyBindings& KeyBindingSet() const override;
  const KeyBindingService& KeyBindingServiceRef() const override;
  KeyBindingUpdateResult UpdateKeyBinding(GameAction action,
                                          engine::input::Key key) override;
  engine::render::Window& Window() override;
  std::shared_ptr<engine::audio::AudioEngine> Audio() override;
  /**
   * @brief Access the asset manager
   */
  ClientAssetManager& Assets() override;
  /**
   * @brief Access shared menu background
   */
  ui::MenuBackground& MenuBackground() override;
  engine::util::Configuration& Config() override;
  void OnPlay() override;
  void OnOpenSettings() override;
  void OnCloseSettings() override;
  void OnQuitApplication() override;
  void OnOpenProfile() override;
  void OnCloseProfile() override;
  void OnQuitToMenu() override;
  void OnGamePause() override;
  void OnGameResume() override;
  void SetConnectionConfig(std::string host, int port, std::string player_name,
                           std::string room_code,
                           std::string room_password = {}) override;
  bool StartConnection() override;
  void RefreshRoomList(std::string host, std::uint16_t port) override;
  void CreateRoom(std::string host, std::uint16_t port,
                  const std::string& room_name, bool is_private,
                  std::string room_password,
                  std::uint16_t max_players) override;
  const std::vector<protocol::RoomSummary>& RoomDirectoryRooms() const override;
  std::string RoomDirectoryStatus() const override;
  std::optional<protocol::CreateRoomResponsePayload>
  ConsumeLastRoomCreation() override;
  engine::ecs::Registry& World() override;
  const engine::ecs::Registry& World() const override;
  bool EnqueueCommand(const protocol::CommandPayload& payload) override;
  std::optional<std::uint32_t> CurrentWave() const override;
  std::optional<float> LatestLatencyMs() const override;
  std::optional<std::uint32_t> LocalPlayerId() const override;
  std::string_view ConnectionStatus() const override;
  bool ConnectionActive() const override;
  PlayerProfile& Profile() override;
  const PlayerProfile& Profile() const override;
  void SaveProfile() override;
  LobbyChatService& ChatService() override;

 private:
  bool Tick(engine::time::TimeDelta dt);
  void StopNetworkSession();
  void HandleNetworkEvents(const NetworkEvents& events);
  void UpdateRuntimeConfig();

  ClientConfig config_{};
  PlayerProfile profile_{};
  std::unique_ptr<ClientRuntime> runtime_;
  std::unique_ptr<ui::MenuBackground> menu_background_;
  std::unique_ptr<SceneManager> scene_manager_;
  std::unique_ptr<AudioController> audio_;
  std::unique_ptr<ClientAssetManager> assets_;
  std::unique_ptr<NetworkSession> network_;
  std::unique_ptr<InputCoordinator> input_;
  std::unique_ptr<LobbyChatService> chat_service_;
  std::optional<std::chrono::steady_clock::time_point> session_start_time_;
};

}  // namespace client

#endif  // CLIENT_APPLICATION_H_
