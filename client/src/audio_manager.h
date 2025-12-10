#ifndef CLIENT_AUDIO_MANAGER_H_
#define CLIENT_AUDIO_MANAGER_H_

#include <optional>
#include <string>
#include <unordered_map>

#include "engine/audio/audio_engine.h"

namespace client {

enum class SoundType { kShot, kExplosion, kHit };

enum class MusicType { kBackground };

/**
 * @brief Simple wrapper around the engine audio subsystem for client assets
 */
class AudioManager {
 public:
  /**
   * @brief Construct an audio manager tied to the provided engine
   */
  explicit AudioManager(engine::audio::AudioEngine& engine);

  /**
   * @brief Preload configured sound effects and music
   */
  void LoadAssets();

  /**
   * @brief Play a sound effect matching the given type
   */
  void PlaySound(SoundType type);

  /**
   * @brief Start looping music for the provided category
   */
  void PlayMusic(MusicType type);

  /**
   * @brief Stop any active music immediately
   */
  void StopMusic();

  /**
   * @brief Fade out active music over the given duration
   */
  void FadeOutMusic(float duration_seconds);

  /**
   * @brief Progress fade timers and apply volume ramps
   */
  void Update(float dt_seconds);

  /**
   * @brief Active music state helper
   */
  bool MusicActive() const;

 private:
  engine::audio::AudioEngine& engine_;
  std::unordered_map<SoundType, std::string> sound_paths_;
  std::unordered_map<MusicType, std::string> music_paths_;
  std::optional<MusicType> current_music_;
  float fade_remaining_{0.0f};
  float fade_duration_{0.0f};
  float initial_music_volume_{1.0f};
  float target_music_volume_{0.65f};
  bool fading_{false};
};

}  // namespace client

#endif  // CLIENT_AUDIO_MANAGER_H_
