#include "audio_controller.h"

#include <array>

#include "audio/audio_manager.h"
#include "audio/sound_effects.h"
#include "constants/client_constants.h"
#include "logging.h"

namespace client {
namespace {

struct MusicCueEntry {
  ClientState state;
  std::optional<MusicType> cue;
};

constexpr std::array<MusicCueEntry, 8> kMusicCueTable{{
    {ClientState::kMainMenu, MusicType::kMainMenu},
    {ClientState::kLobby, MusicType::kMainMenu},
    {ClientState::kSettings, MusicType::kMainMenu},
    {ClientState::kConnecting, std::nullopt},
    {ClientState::kInGame, MusicType::kBackground},
    {ClientState::kPaused, MusicType::kBackground},
    {ClientState::kGameOver, std::nullopt},
    {ClientState::kDisconnected, std::nullopt},
}};

std::optional<MusicType> ResolveMusicCue(
    ClientState state, std::optional<ClientState> settings_return_state) {
  if (state == ClientState::kSettings && settings_return_state.has_value()) {
    const ClientState return_state = *settings_return_state;
    if (return_state == ClientState::kInGame ||
        return_state == ClientState::kPaused) {
      state = return_state;
    }
  }
  for (const auto& entry : kMusicCueTable) {
    if (entry.state == state) {
      return entry.cue;
    }
  }
  return std::nullopt;
}

}  // namespace

AudioController::AudioController() = default;

AudioController::~AudioController() = default;

void AudioController::Initialize(engine::audio::AudioEngine& engine) {
  audio_manager_ = std::make_unique<AudioManager>(engine);
  audio_manager_->LoadAssets();
  sound_effects_ = std::make_unique<SoundEffects>(engine);
  sound_effects_->LoadAssets();
  LogLifecycle(engine::util::LogLevel::kInfo, "Audio manager initialized");
}

void AudioController::StopMusic() {
  if (audio_manager_) {
    audio_manager_->StopMusic();
  }
}

void AudioController::Reset() {
  if (sound_effects_) {
    sound_effects_->Reset();
  }
}

void AudioController::Update(engine::time::TimeDelta dt, ClientState state,
                             std::optional<ClientState> settings_return_state,
                             bool connected, bool transport_running) {
  if (!audio_manager_) {
    return;
  }

  const auto desired_music = ResolveMusicCue(state, settings_return_state);
  const auto active_music = audio_manager_->ActiveMusic();
  const bool menu_music_active =
      active_music.has_value() && active_music.value() == MusicType::kMainMenu;
  const bool wants_menu_music =
      desired_music.has_value() && *desired_music == MusicType::kMainMenu;
  const bool wants_game_music =
      desired_music.has_value() && *desired_music == MusicType::kBackground;

  if (wants_menu_music) {
    music_allowed_ = false;
    music_blocked_ = false;
    if (!menu_music_active) {
      audio_manager_->PlayMusic(MusicType::kMainMenu);
    }
    audio_manager_->Update(dt.as_seconds());
    was_connected_ = connected;
    return;
  }

  if (menu_music_active) {
    audio_manager_->StopMusic();
  }

  if (!wants_game_music) {
    music_allowed_ = false;
  }

  if (connected && !music_blocked_ && wants_game_music) {
    music_allowed_ = true;
  }

  const bool lost_connection =
      (!connected && was_connected_) || !transport_running;
  if (lost_connection) {
    music_allowed_ = false;
    music_blocked_ = false;
    if (audio_manager_->MusicActive()) {
      audio_manager_->FadeOutMusic(constants::client::kDisconnectFadeSeconds);
    }
  }

  if (wants_game_music && music_allowed_ && !music_blocked_ &&
      !audio_manager_->MusicActive()) {
    audio_manager_->PlayMusic(MusicType::kBackground);
  }

  audio_manager_->Update(dt.as_seconds());
  was_connected_ = connected;
}

void AudioController::OnSnapshotApplied(const engine::ecs::Registry& registry) {
  if (sound_effects_) {
    sound_effects_->OnSnapshotApplied(registry);
  }
}

void AudioController::OnPlayerDeath() {
  if (sound_effects_) {
    sound_effects_->OnPlayerDeath();
  }
  HandleGameOverAudio();
}

void AudioController::HandleGameOverAudio() {
  if (!audio_manager_) {
    return;
  }
  music_allowed_ = false;
  music_blocked_ = true;
  audio_manager_->FadeOutMusic(constants::client::kGameOverFadeSeconds);
}

}  // namespace client
