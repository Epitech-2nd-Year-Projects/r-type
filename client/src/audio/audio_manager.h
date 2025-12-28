#ifndef CLIENT_AUDIO_MANAGER_H_
#define CLIENT_AUDIO_MANAGER_H_

#include <optional>
#include <string>
#include <unordered_map>

#include "engine/audio/audio_engine.h"

namespace client {

enum class MusicType { kMainMenu, kBackground };

/**
 * @brief Simple wrapper around the engine audio subsystem for client music
 */
class AudioManager {
 public:
  /**
   * @brief Construct an audio manager tied to the provided engine
   */
  explicit AudioManager(engine::audio::AudioEngine& engine);

  /**
   * @brief Preload configured music files
   */
  void LoadAssets();

  /**
   * @brief Start looping music for the provided category
   *
   * @param type Music entry to start
   */
  void PlayMusic(MusicType type);

  /**
   * @brief Stop any active music immediately
   */
  void StopMusic();

  /**
   * @brief Fade out active music over the given duration
   *
   * @param duration_seconds Time in seconds to ramp volume down to silence
   */
  void FadeOutMusic(float duration_seconds);

  /**
   * @brief Progress fade timers and apply volume ramps
   *
   * @param dt_seconds Delta time in seconds since the previous update
   */
  void Update(float dt_seconds);

  /**
   * @brief Active music state helper
   *
   * @return true when a music track is currently playing
   */
  bool MusicActive() const;

  /**
   * @brief Currently playing music entry if available
   *
   * @return MusicType identifier for the active track when present
   */
  std::optional<MusicType> ActiveMusic() const;

 private:
  static constexpr float kDefaultMusicVolume = 0.65f;

  engine::audio::AudioEngine& engine_;
  std::unordered_map<MusicType, std::string> music_paths_;
  std::optional<MusicType> current_music_;
  float fade_remaining_{0.0f};
  float fade_duration_{0.0f};
  float initial_music_volume_{0.0f};
  float target_music_volume_{kDefaultMusicVolume};
  bool fading_{false};
};

}  // namespace client

#endif  // CLIENT_AUDIO_MANAGER_H_
