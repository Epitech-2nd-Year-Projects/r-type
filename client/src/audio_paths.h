/**
 * @file audio_paths.h
 * @brief Audio asset path resolution helpers
 */

#ifndef CLIENT_AUDIO_PATHS_H_
#define CLIENT_AUDIO_PATHS_H_

#include <string>
#include <string_view>

namespace client {

/**
 * @brief Resolve an audio asset path relative to common project roots
 *
 * Searches for relative_path appended to ASSET_ROOT, the working directory,
 * and up to five parent directories.
 */
std::string ResolveAssetPath(std::string_view relative_path);

}  // namespace client

#endif  // CLIENT_AUDIO_PATHS_H_
