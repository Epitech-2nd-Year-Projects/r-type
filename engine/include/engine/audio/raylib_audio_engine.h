#ifndef ENGINE_ENGINE_AUDIO_RAYLIB_AUDIO_ENGINE_H_
#define ENGINE_ENGINE_AUDIO_RAYLIB_AUDIO_ENGINE_H_

#include <memory>

#include "engine/audio/audio_engine.h"

namespace engine::audio {

/**
 * @brief Create a Raylib-based audio engine instance.
 */
std::unique_ptr<AudioEngine> CreateRaylibAudioEngine();

}  // namespace engine::audio

#endif  // ENGINE_ENGINE_AUDIO_RAYLIB_AUDIO_ENGINE_H_
