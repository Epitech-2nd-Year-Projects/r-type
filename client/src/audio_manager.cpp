#include "audio_manager.h"

#include "logging.h"

namespace client {

AudioManager::AudioManager(engine::audio::AudioEngine& engine)
    : engine_(engine) {
  sound_paths_[SoundType::kShot] = "assets/audio/test.wav";
  sound_paths_[SoundType::kExplosion] = "assets/audio/test.wav";
  sound_paths_[SoundType::kHit] = "assets/audio/test.wav";
  music_paths_[MusicType::kBackground] = "assets/audio/test.ogg";
}

void AudioManager::LoadAssets() {
  LogLifecycle(engine::util::LogLevel::kInfo, "Preloading audio assets...");
  const float original_sfx_volume = engine_.GetSfxVolume();
  engine_.SetSfxVolume(0.0f);

  for (const auto& [type, path] : sound_paths_) {
    engine_.PlaySoundEffect(path);
  }

  engine_.SetSfxVolume(original_sfx_volume);
}

void AudioManager::PlaySound(SoundType type) {
  if (sound_paths_.count(type)) {
    engine_.PlaySoundEffect(sound_paths_.at(type));
  }
}

void AudioManager::PlayMusic(MusicType type) {
  if (music_paths_.count(type)) {
    engine_.PlayMusic(music_paths_.at(type));
  }
}

void AudioManager::StopMusic() { engine_.StopMusic(); }

}  // namespace client
