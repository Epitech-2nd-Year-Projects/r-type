/**
 * @file scene_manager.h
 * @brief State driven scene switching for the client
 */

#ifndef CLIENT_SCENE_MANAGER_H_
#define CLIENT_SCENE_MANAGER_H_

#include <memory>
#include <optional>
#include <string>

#include "client_state.h"
#include "engine/time/time_delta.h"
#include "protocol/gameplay_ping.h"

namespace engine::render {
class Renderer2D;
}  // namespace engine::render

namespace client {

class ClientContext;
class Scene;

/**
 * @brief Scene manager with state transitions
 */
class SceneManager {
 public:
  /**
   * @brief Construct a scene manager bound to a client context
   * @param context Client context reference
   */
  explicit SceneManager(ClientContext& context);

  /**
   * @brief Destroy the scene manager
   */
  ~SceneManager();

  SceneManager(const SceneManager&) = delete;
  SceneManager& operator=(const SceneManager&) = delete;
  SceneManager(SceneManager&&) = delete;
  SceneManager& operator=(SceneManager&&) = delete;

  /**
   * @brief Initialize the first scene
   * @param initial_state First state to apply
   */
  void Initialize(ClientState initial_state);

  /**
   * @brief Update the active scene
   * @param dt Frame delta
   */
  void Update(engine::time::TimeDelta dt);

  /**
   * @brief Draw the active scene
   * @param renderer Renderer instance
   */
  void Draw(engine::render::Renderer2D& renderer);

  /**
   * @brief Commit a pending scene swap
   */
  void CommitSceneChange();

  /**
   * @brief Check if the scene is capturing input
   * @return True when input should be blocked
   */
  bool IsInputCaptured() const;

  /**
   * @brief Access current state
   */
  ClientState state() const { return state_; }

  /**
   * @brief Access the settings return state
   */
  std::optional<ClientState> settings_return_state() const {
    return settings_return_state_;
  }

  /**
   * @brief Access the active scene
   */
  std::shared_ptr<Scene> CurrentScene() const { return current_scene_; }

  /**
   * @brief Switch to the lobby flow
   */
  void OnPlay();

  /**
   * @brief Mark a connection success
   */
  void OnConnected();

  /**
   * @brief Mark a connection failure
   * @param reason Failure reason
   */
  void OnConnectionFailed(const std::string& reason);

  /**
   * @brief Mark a game start
   */
  void OnGameStart();

  /**
   * @brief Pause the game
   */
  void OnGamePause();

  /**
   * @brief Resume the game
   */
  void OnGameResume();

  /**
   * @brief Enter game over state with stats
   * @param stats Final stats
   */
  void OnGameOver(const GameOverStats& stats);

  /**
   * @brief Enter game over state with defaults
   */
  void OnGameOver();

  /**
   * @brief Disconnect to the main menu
   * @param reason Disconnect reason
   */
  void OnDisconnect(std::string reason);

  /**
   * @brief Return to the main menu
   */
  void OnQuitToMenu();

  /**
   * @brief Open the settings menu
   */
  void OnOpenSettings();

  /**
   * @brief Open the audio settings menu
   */
  void OnOpenAudioSettings();

  /**
   * @brief Open the video settings menu
   */
  void OnOpenVideoSettings();

  /**
   * @brief Close the audio settings menu
   */
  void OnCloseAudioSettings();

  /**
   * @brief Close the video settings menu
   */
  void OnCloseVideoSettings();

  /**
   * @brief Close the settings menu
   */
  void OnCloseSettings();

  /**
   * @brief Open the profile editor
   */
  void OnOpenProfile();

  /**
   * @brief Close the profile editor
   */
  void OnCloseProfile();
  
  /**
   * @brief Handle incoming gameplay ping
   */
  void OnGameplayPing(const protocol::GameplayPingPayload& ping);

  /**
   * @brief Transition to a new state
   * @param next_state State to apply
   * @param reason Optional reason
   * @return True when transition is allowed
   */
  bool TransitionTo(ClientState next_state, std::string reason = {});

  /**
   * @brief Check if a transition is allowed
   * @param next_state State to test
   * @return True when allowed
   */
  bool CanTransitionTo(ClientState next_state) const;

 private:
  void SwitchScene(std::shared_ptr<Scene> scene);
  void ApplyState(ClientState next_state, std::string reason);
  bool CanTransition(ClientState next_state) const;

  ClientContext& context_;
  std::shared_ptr<Scene> current_scene_;
  std::shared_ptr<Scene> pending_scene_;
  ClientState state_{ClientState::kSplash};
  std::optional<ClientState> settings_return_state_;
  std::string disconnect_reason_;
  GameOverStats last_game_stats_{};
};

}  // namespace client

#endif  // CLIENT_SCENE_MANAGER_H_
