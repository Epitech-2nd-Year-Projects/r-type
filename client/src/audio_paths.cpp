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
  auto normalize = [](const std::filesystem::path& path) {
    auto normalized = path.lexically_normal();
    normalized.make_preferred();
    return normalized.string();
  };

  try {
    if (const char* asset_root = std::getenv("ASSET_ROOT")) {
      std::filesystem::path candidate =
          std::filesystem::path(asset_root) / relative_path;
      if (std::filesystem::exists(candidate)) {
        return normalize(candidate);
      }
    }

    std::filesystem::path cwd_candidate(relative_path);
    if (std::filesystem::exists(cwd_candidate)) {
      return normalize(std::filesystem::absolute(cwd_candidate));
    }

    std::filesystem::path cursor = std::filesystem::current_path();
    for (int i = 0; i < 5 && !cursor.empty(); ++i) {
      std::filesystem::path candidate = cursor / relative_path;
      if (std::filesystem::exists(candidate)) {
        return normalize(candidate);
      }
      cursor = cursor.parent_path();
    }

    LogLifecycle(engine::util::LogLevel::kError,
                 "Failed to resolve asset path: " + std::string(relative_path));
  } catch (const std::filesystem::filesystem_error& e) {
    LogLifecycle(engine::util::LogLevel::kError,
                 std::string("Filesystem error in ResolveAssetPath: ") +
                     e.what() +
                     " (while resolving: " + std::string(relative_path) + ")");
  } catch (const std::exception& e) {
    LogLifecycle(engine::util::LogLevel::kError,
                 std::string("Exception in ResolveAssetPath: ") + e.what() +
                     " (while resolving: " + std::string(relative_path) + ")");
  }

  return std::string();
}

}  // namespace client
