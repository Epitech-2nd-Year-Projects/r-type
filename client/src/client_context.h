/**
 * @file client_context.h
 * @brief Client service access for scenes
 */

#ifndef CLIENT_CLIENT_CONTEXT_H_
#define CLIENT_CLIENT_CONTEXT_H_

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "engine/math/vector2.h"
#include "engine/render/window.h"
#include "engine/util/config.h"
#include "input/input_layer.h"
#include "input/key_binding_service.h"
#include "player_profile.h"
#include "protocol/command.h"
#include "protocol/lobby.h"

namespace engine::audio {
class AudioEngine;
}  // namespace engine::audio

namespace engine::ecs {
class Registry;
}  // namespace engine::ecs

namespace engine::input {
class InputManager;
}  // namespace engine::input

namespace engine::render {
class Renderer2D;
class Window;
}  // namespace engine::render

namespace engine::util {
class Configuration;
}  // namespace engine::util

namespace client {

class ClientAssetManager;
namespace ui {
class MenuBackground;
}

/**
 * @brief Service container for client scenes
 */
class ClientContext {
 public:
  virtual ~ClientContext() = default;

  /**
   * @brief Access the renderer
   */
  virtual engine::render::Renderer2D& Renderer() = 0;

  /**
   * @brief Access the input manager
   */
  virtual engine::input::InputManager& Input() = 0;

  /**
   * @brief Access key binding set
   */
  virtual const KeyBindings& KeyBindingSet() const = 0;

  /**
   * @brief Access key binding service
   */
  virtual const KeyBindingService& KeyBindingServiceRef() const = 0;

  /**
   * @brief Update a key binding and persist it
   * @param action Gameplay action
   * @param key Input key
   */
  virtual KeyBindingUpdateResult UpdateKeyBinding(GameAction action,
                                                  engine::input::Key key) = 0;

  /**
   * @brief Access the active window
   */
  virtual engine::render::Window& Window() = 0;

  /**
   * @brief Access the virtual render size
   */
  virtual engine::math::Vector2i RenderSize() const = 0;

  /**
   * @brief Access the audio engine when available
   */
  virtual std::shared_ptr<engine::audio::AudioEngine> Audio() = 0;

  /**
   * @brief Update audio volume settings and persist them
   */
  virtual void SetAudioVolumes(float master_volume, float music_volume,
                               float sfx_volume) = 0;

  /**
   * @brief Update video settings and persist them
   */
  virtual void SetVideoSettings(int resolution_width, int resolution_height,
                                bool fullscreen, bool vsync,
                                int target_fps) = 0;

  /**
   * @brief Access the asset manager
   */
  virtual ClientAssetManager& Assets() = 0;

  /**
   * @brief Access shared menu background
   */
  virtual ui::MenuBackground& MenuBackground() = 0;

  /**
   * @brief Access runtime configuration
   */
  virtual engine::util::Configuration& Config() = 0;

  /**
   * @brief Switch to the play flow
   */
  virtual void OnPlay() = 0;

  /**
   * @brief Open the settings menu
   */
  virtual void OnOpenSettings() = 0;

  /**
   * @brief Open the audio settings menu
   */
  virtual void OnOpenAudioSettings() = 0;

  /**
   * @brief Open the video settings menu
   */
  virtual void OnOpenVideoSettings() = 0;

  /**
   * @brief Close the settings menu
   */
  virtual void OnCloseSettings() = 0;

  /**
   * @brief Close the audio settings menu
   */
  virtual void OnCloseAudioSettings() = 0;

  /**
   * @brief Close the video settings menu
   */
  virtual void OnCloseVideoSettings() = 0;

  /**
   * @brief Quit the application
   */
  virtual void OnQuitApplication() = 0;

  /**
   * @brief Open the profile editor
   */
  virtual void OnOpenProfile() = 0;

  /**
   * @brief Close the profile editor
   */
  virtual void OnCloseProfile() = 0;

  /**
   * @brief Quit to the main menu
   */
  virtual void OnQuitToMenu() = 0;

  /**
   * @brief Pause the game
   */
  virtual void OnGamePause() = 0;

  /**
   * @brief Resume the game
   */
  virtual void OnGameResume() = 0;

  /**
   * @brief Update connection configuration
   */
  virtual void SetConnectionConfig(std::string host, int port,
                                   std::string player_name,
                                   std::string room_code,
                                   std::string room_password = {}) = 0;

  /**
   * @brief Begin the connection handshake
   */
  virtual bool StartConnection() = 0;

  /**
   * @brief Request a new room list
   */
  virtual void RefreshRoomList(std::string host, std::uint16_t port) = 0;

  /**
   * @brief Ask the server to create a room
   */
  virtual void CreateRoom(std::string host, std::uint16_t port,
                          const std::string& room_name, bool is_private,
                          std::string room_password,
                          std::uint16_t max_players) = 0;

  /**
   * @brief Snapshot of available rooms
   */
  virtual const std::vector<protocol::RoomSummary>& RoomDirectoryRooms()
      const = 0;

  /**
   * @brief Room directory status text
   */
  virtual std::string RoomDirectoryStatus() const = 0;

  /**
   * @brief Latest room creation response
   */
  virtual std::optional<protocol::CreateRoomResponsePayload>
  ConsumeLastRoomCreation() = 0;

  /**
   * @brief Access the ECS world
   */
  virtual engine::ecs::Registry& World() = 0;

  /**
   * @brief Access the ECS world read only view
   */
  virtual const engine::ecs::Registry& World() const = 0;

  /**
   * @brief Enqueue a server command
   */
  virtual bool EnqueueCommand(const protocol::CommandPayload& payload) = 0;

  /**
   * @brief Current wave number
   */
  virtual std::optional<std::uint32_t> CurrentWave() const = 0;

  /**
   * @brief Latest latency measurement in milliseconds
   */
  virtual std::optional<float> LatestLatencyMs() const = 0;

  /**
   * @brief Current local player id
   */
  virtual std::optional<std::uint32_t> LocalPlayerId() const = 0;

  /**
   * @brief Connection status message
   */
  virtual std::string_view ConnectionStatus() const = 0;

  /**
   * @brief Connection active flag
   */
  virtual bool ConnectionActive() const = 0;

  /**
   * @brief Access the player profile
   */
  virtual PlayerProfile& Profile() = 0;

  /**
   * @brief Access the player profile (const)
   */
  virtual const PlayerProfile& Profile() const = 0;

  /**
   * @brief Persist the player profile to disk
   */
  virtual void SaveProfile() = 0;
};

}  // namespace client

#endif  // CLIENT_CLIENT_CONTEXT_H_
