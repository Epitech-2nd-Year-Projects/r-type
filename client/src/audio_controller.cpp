#include "audio_controller.h"

#include "audio_manager.h"
#include "constants/client_constants.h"
#include "logging.h"
#include "sound_effects.h"

namespace client {

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

  const bool settings_from_gameplay =
      state == ClientState::kSettings && settings_return_state.has_value() &&
      (*settings_return_state == ClientState::kInGame ||
       *settings_return_state == ClientState::kPaused);

  const bool in_menu =
      state == ClientState::kMainMenu || state == ClientState::kLobby ||
      (state == ClientState::kSettings && !settings_from_gameplay);
  const auto active_music = audio_manager_->ActiveMusic();
  const bool menu_music_active =
      active_music.has_value() && active_music.value() == MusicType::kMainMenu;

  if (in_menu) {
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

  if (connected && !music_blocked_) {
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

  if (music_allowed_ && !music_blocked_ && !audio_manager_->MusicActive()) {
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
