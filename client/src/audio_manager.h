#ifndef CLIENT_AUDIO_MANAGER_H_
#define CLIENT_AUDIO_MANAGER_H_

#include <string>
#include <unordered_map>

#include "engine/audio/audio_engine.h"

namespace client {

enum class SoundType { kShot, kExplosion, kHit };

enum class MusicType { kBackground };

class AudioManager {
 public:
  explicit AudioManager(engine::audio::AudioEngine& engine);

  void LoadAssets();

  void PlaySound(SoundType type);
  void PlayMusic(MusicType type);
  void StopMusic();

 private:
  engine::audio::AudioEngine& engine_;
  std::unordered_map<SoundType, std::string> sound_paths_;
  std::unordered_map<MusicType, std::string> music_paths_;
};

}  // namespace client

#endif  // CLIENT_AUDIO_MANAGER_H_
