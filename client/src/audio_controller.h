/**
 * @file audio_controller.h
 * @brief Client audio orchestration
 */

#ifndef CLIENT_AUDIO_CONTROLLER_H_
#define CLIENT_AUDIO_CONTROLLER_H_

#include <memory>
#include <optional>

#include "client_state.h"
#include "engine/time/time_delta.h"

namespace engine::audio {
class AudioEngine;
}  // namespace engine::audio

namespace engine::ecs {
class Registry;
}  // namespace engine::ecs

namespace client {

class AudioManager;
class SoundEffects;

/**
 * @brief Audio controller for music and sound effects
 */
class AudioController {
 public:
  /**
   * @brief Construct the audio controller
   */
  AudioController();

  /**
   * @brief Destroy the audio controller
   */
  ~AudioController();

  AudioController(const AudioController&) = delete;
  AudioController& operator=(const AudioController&) = delete;
  AudioController(AudioController&&) = delete;
  AudioController& operator=(AudioController&&) = delete;

  /**
   * @brief Initialize audio resources
   * @param engine Audio engine reference
   */
  void Initialize(engine::audio::AudioEngine& engine);

  /**
   * @brief Stop active music
   */
  void StopMusic();

  /**
   * @brief Reset sound effect state
   */
  void Reset();

  /**
   * @brief Update music state
   * @param dt Frame delta
   * @param state Client state
   * @param settings_return_state Optional settings return state
   * @param connected Connection state flag
   * @param transport_running Transport active flag
   */
  void Update(engine::time::TimeDelta dt, ClientState state,
              std::optional<ClientState> settings_return_state, bool connected,
              bool transport_running);

  /**
   * @brief React to a snapshot update
   * @param registry World registry
   */
  void OnSnapshotApplied(const engine::ecs::Registry& registry);

  /**
   * @brief React to a player death event
   */
  void OnPlayerDeath();

 private:
  void HandleGameOverAudio();

  std::unique_ptr<AudioManager> audio_manager_;
  std::unique_ptr<SoundEffects> sound_effects_;
  bool music_allowed_{false};
  bool music_blocked_{false};
  bool was_connected_{false};
};

}  // namespace client

#endif  // CLIENT_AUDIO_CONTROLLER_H_
