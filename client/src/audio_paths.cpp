/**
 * @file audio_paths.cpp
 * @brief Audio asset path resolution helpers
 */

#include "audio_paths.h"

#include <cstdlib>
#include <filesystem>
#include <string_view>

#include "logging.h"

namespace client {

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

}  // namespace client
