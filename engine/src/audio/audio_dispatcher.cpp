#include "engine/audio/audio_dispatcher.h"

namespace engine::audio {

AudioDispatcher::AudioDispatcher(AudioCommandQueue& queue) : queue_(queue) {}

void AudioDispatcher::PlaySound(std::string_view path, float volume) {
  queue_.Push(PlaySoundCommand{std::string(path), volume});
}

void AudioDispatcher::PlayMusic(std::string_view path, bool loop) {
  queue_.Push(PlayMusicCommand{std::string(path), loop});
}

void AudioDispatcher::StopMusic() { queue_.Push(StopMusicCommand{}); }

void AudioDispatcher::SetMasterVolume(float volume) {
  queue_.Push(SetVolumeCommand{SetVolumeCommand::Target::kMaster, volume});
}

void AudioDispatcher::SetMusicVolume(float volume) {
  queue_.Push(SetVolumeCommand{SetVolumeCommand::Target::kMusic, volume});
}

void AudioDispatcher::SetSfxVolume(float volume) {
  queue_.Push(SetVolumeCommand{SetVolumeCommand::Target::kSfx, volume});
}

}  // namespace engine::audio
