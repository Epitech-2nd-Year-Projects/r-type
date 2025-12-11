#include "audio_manager.h"

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <string_view>

#include "logging.h"

namespace client {
namespace {

std::string ResolveAssetPath(std::string_view relative_path) {
  if (const char* asset_root = std::getenv("ASSET_ROOT")) {
    std::filesystem::path candidate =
        std::filesystem::path(asset_root) / relative_path;
    if (std::filesystem::exists(candidate)) {
      return candidate.string();
    }
  }

  std::filesystem::path cwd_candidate(relative_path);
  if (std::filesystem::exists(cwd_candidate)) {
    return std::filesystem::absolute(cwd_candidate).string();
  }

  std::filesystem::path cursor = std::filesystem::current_path();
  for (int i = 0; i < 5 && !cursor.empty(); ++i) {
    std::filesystem::path candidate = cursor / relative_path;
    if (std::filesystem::exists(candidate)) {
      return candidate.string();
    }
    cursor = cursor.parent_path();
  }

  LogLifecycle(engine::util::LogLevel::kError,
               "Failed to resolve asset path: " + std::string(relative_path));
  return std::string();
}

}  // namespace

AudioManager::AudioManager(engine::audio::AudioEngine& engine)
    : engine_(engine) {
  music_paths_[MusicType::kBackground] =
      ResolveAssetPath("assets/background_music.ogg");
}

void AudioManager::LoadAssets() {
  LogLifecycle(engine::util::LogLevel::kInfo, "Preloading audio assets...");
  const float original_music_volume = engine_.GetMusicVolume();
  engine_.SetMusicVolume(0.0f);
  for (const auto& [type, path] : music_paths_) {
    engine_.PlayMusic(path);
    engine_.StopMusic();
  }
  engine_.SetMusicVolume(original_music_volume);
}

void AudioManager::PlayMusic(MusicType type) {
  const auto it = music_paths_.find(type);
  if (it == music_paths_.end() || it->second.empty()) {
    LogLifecycle(engine::util::LogLevel::kError,
                 "No audio path available for requested music type");
    return;
  }
  fading_ = false;
  fade_remaining_ = 0.0f;
  fade_duration_ = 0.0f;
  current_music_ = type;
  engine_.SetMusicVolume(default_music_volume_);
  engine_.PlayMusic(it->second);
}

void AudioManager::StopMusic() {
  engine_.StopMusic();
  fading_ = false;
  fade_remaining_ = 0.0f;
  fade_duration_ = 0.0f;
  current_music_.reset();
}

void AudioManager::FadeOutMusic(float duration_seconds) {
  if (!current_music_.has_value()) {
    return;
  }
  if (duration_seconds <= 0.0f) {
    StopMusic();
    return;
  }
  fading_ = true;
  fade_duration_ = duration_seconds;
  fade_remaining_ = duration_seconds;
  initial_music_volume_ = engine_.GetMusicVolume();
}

void AudioManager::Update(float dt_seconds) {
  if (!fading_) {
    return;
  }
  fade_remaining_ = std::max(0.0f, fade_remaining_ - dt_seconds);
  const float t =
      fade_duration_ > 0.0f ? fade_remaining_ / fade_duration_ : 0.0f;
  engine_.SetMusicVolume(initial_music_volume_ * t);
  if (fade_remaining_ <= 0.0f) {
    StopMusic();
  }
}

bool AudioManager::MusicActive() const { return current_music_.has_value(); }

}  // namespace client
