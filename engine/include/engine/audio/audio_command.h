#ifndef ENGINE_AUDIO_AUDIO_COMMAND_H_
#define ENGINE_AUDIO_AUDIO_COMMAND_H_

#include <string>
#include <variant>

#include "engine/util/thread_safe_queue.h"

namespace engine::audio {

struct PlaySoundCommand {
  std::string path;
  float volume{1.0f};
};

struct PlayMusicCommand {
  std::string path;
  bool looping{true};
};

struct StopMusicCommand {};

struct SetVolumeCommand {
  enum class Target { kMaster, kMusic, kSfx };
  Target target;
  float volume;
};

using AudioCommand = std::variant<PlaySoundCommand, PlayMusicCommand,
                                  StopMusicCommand, SetVolumeCommand>;

using AudioCommandQueue = util::ThreadSafeQueue<AudioCommand>;

}  // namespace engine::audio

#endif  // ENGINE_AUDIO_AUDIO_COMMAND_H_
