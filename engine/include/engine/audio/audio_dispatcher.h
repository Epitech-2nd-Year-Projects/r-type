#ifndef ENGINE_AUDIO_AUDIO_DISPATCHER_H_
#define ENGINE_AUDIO_AUDIO_DISPATCHER_H_

#include <string_view>

#include "engine/audio/audio_command.h"

namespace engine::audio {

class AudioDispatcher {
 public:
  explicit AudioDispatcher(AudioCommandQueue& queue);

  void PlaySound(std::string_view path, float volume = 1.0f);
  void PlayMusic(std::string_view path, bool loop = true);
  void StopMusic();
  void SetMasterVolume(float volume);
  void SetMusicVolume(float volume);
  void SetSfxVolume(float volume);

 private:
  AudioCommandQueue& queue_;
};

}  // namespace engine::audio

#endif  // ENGINE_AUDIO_AUDIO_DISPATCHER_H_
