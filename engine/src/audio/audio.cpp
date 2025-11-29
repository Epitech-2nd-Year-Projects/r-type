#include <raylib.h>

#include <algorithm>
#include <memory>
#include <string>
#include <unordered_map>

#include "engine/audio/raylib_audio_engine.h"

namespace engine::audio {
namespace {

float ClampVolume(float volume) { return std::clamp(volume, 0.0f, 1.0f); }

size_t& EngineInstanceCount() {
  static size_t count = 0;
  return count;
}

class RaylibAudioEngine final : public AudioEngine {
 public:
  RaylibAudioEngine() {
    if (EngineInstanceCount() == 0) {
      ::InitAudioDevice();
    }
    EngineInstanceCount() += 1;
    ::SetMasterVolume(master_volume_);
  }

  ~RaylibAudioEngine() override {
    StopMusicInternal();
    UnloadAllSounds();

    EngineInstanceCount() -= 1;
    if (EngineInstanceCount() == 0 && ::IsAudioDeviceReady()) {
      ::CloseAudioDevice();
    }
  }

  void PlaySoundEffect(const std::string& path) override {
    if (!::IsAudioDeviceReady()) {
      return;
    }

    ::Sound* sound = LoadSoundCached(path);
    if (sound == nullptr) {
      return;
    }

    ::SetSoundVolume(*sound, sfx_volume_);
    ::PlaySound(*sound);
  }

  void PlayMusic(const std::string& path) override {
    if (!::IsAudioDeviceReady()) {
      return;
    }

    StopMusicInternal();

    current_music_ = ::LoadMusicStream(path.c_str());
    if (current_music_.stream.buffer == nullptr) {
      return;
    }

    current_music_.looping = true;
    ::SetMusicVolume(current_music_, music_volume_);
    ::PlayMusicStream(current_music_);
    music_loaded_ = true;
  }

  void StopMusic() override { StopMusicInternal(); }

  void Update() override {
    if (music_loaded_) {
      ::UpdateMusicStream(current_music_);
    }
  }

  void SetMasterVolume(float volume) override {
    master_volume_ = ClampVolume(volume);
    ::SetMasterVolume(master_volume_);
  }

  void SetMusicVolume(float volume) override {
    music_volume_ = ClampVolume(volume);
    if (music_loaded_) {
      ::SetMusicVolume(current_music_, music_volume_);
    }
  }

  void SetSfxVolume(float volume) override {
    sfx_volume_ = ClampVolume(volume);
  }

  float GetMasterVolume() const override { return master_volume_; }
  float GetMusicVolume() const override { return music_volume_; }
  float GetSfxVolume() const override { return sfx_volume_; }

 private:
  void StopMusicInternal() {
    if (!music_loaded_) {
      return;
    }

    ::StopMusicStream(current_music_);
    ::UnloadMusicStream(current_music_);
    music_loaded_ = false;
  }

  void UnloadAllSounds() {
    for (auto& entry : sfx_cache_) {
      ::UnloadSound(entry.second);
    }
    sfx_cache_.clear();
  }

  ::Sound* LoadSoundCached(const std::string& path) {
    auto [it, inserted] = sfx_cache_.try_emplace(path);
    if (inserted) {
      it->second = ::LoadSound(path.c_str());
      if (it->second.frameCount == 0) {
        sfx_cache_.erase(it);
        return nullptr;
      }
    }
    return &it->second;
  }

  float master_volume_ = 1.0f;
  float music_volume_ = 1.0f;
  float sfx_volume_ = 1.0f;
  bool music_loaded_ = false;
  ::Music current_music_{};
  std::unordered_map<std::string, ::Sound> sfx_cache_;
};

}  // namespace

std::unique_ptr<AudioEngine> CreateRaylibAudioEngine() {
  return std::make_unique<RaylibAudioEngine>();
}

}  // namespace engine::audio
