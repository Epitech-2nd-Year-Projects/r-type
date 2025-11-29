#ifndef ENGINE_ENGINE_AUDIO_AUDIO_ENGINE_H_
#define ENGINE_ENGINE_AUDIO_AUDIO_ENGINE_H_

#include <string>

namespace engine::audio {

/**
 * @brief Abstract audio engine capable of playing music and sound effects.
 */
class AudioEngine {
 public:
  virtual ~AudioEngine() = default;

  /**
   * @brief Play a short sound effect once.
   *
   * Implementations may cache file contents after the first call.
   */
  virtual void PlaySoundEffect(const std::string& path) = 0;

  /**
   * @brief Play (and loop) background music from a file, replacing any
   * currently playing track.
   */
  virtual void PlayMusic(const std::string& path) = 0;

  /**
   * @brief Stop the currently playing music, if any.
   */
  virtual void StopMusic() = 0;

  /**
   * @brief Update internal streaming state. Must be called regularly (e.g. once
   * per frame) to keep music streams alive.
   */
  virtual void Update() = 0;

  /**
   * @brief Set the global volume multiplier applied to both music and SFX.
   */
  virtual void SetMasterVolume(float volume) = 0;

  /**
   * @brief Set the volume multiplier for background music.
   */
  virtual void SetMusicVolume(float volume) = 0;

  /**
   * @brief Set the volume multiplier for sound effects.
   */
  virtual void SetSfxVolume(float volume) = 0;

  /**
   * @brief Get the current master volume.
   */
  virtual float GetMasterVolume() const = 0;

  /**
   * @brief Get the current music volume.
   */
  virtual float GetMusicVolume() const = 0;

  /**
   * @brief Get the current sound effects volume.
   */
  virtual float GetSfxVolume() const = 0;
};

}  // namespace engine::audio

#endif  // ENGINE_ENGINE_AUDIO_AUDIO_ENGINE_H_
