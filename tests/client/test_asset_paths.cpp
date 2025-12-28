#include <gtest/gtest.h>

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>

#include "audio_paths.h"

namespace {

class ScopedEnvVar {
 public:
  ScopedEnvVar(const char* key, const std::string& value) : key_(key) {
    const char* existing = std::getenv(key);
    if (existing) {
      had_value_ = true;
      old_value_ = existing;
    }
    Set(value);
  }

  ~ScopedEnvVar() { Restore(); }

 private:
  void Set(const std::string& value) const {
#if defined(_WIN32)
    _putenv_s(key_.c_str(), value.c_str());
#else
    setenv(key_.c_str(), value.c_str(), 1);
#endif
  }

  void Unset() const {
#if defined(_WIN32)
    _putenv_s(key_.c_str(), "");
#else
    unsetenv(key_.c_str());
#endif
  }

  void Restore() const {
    if (had_value_) {
      Set(old_value_);
      return;
    }
    Unset();
  }

  std::string key_;
  bool had_value_{false};
  std::string old_value_{};
};

}

TEST(AssetPathTest, ResolvesFromCustomAssetRoot) {
  const auto temp_root =
      std::filesystem::temp_directory_path() / "rtype_asset_root";
  std::filesystem::create_directories(temp_root / "assets");
  const auto file_path = temp_root / "assets" / "sample.txt";
  std::ofstream(file_path.string()) << "data";

  ScopedEnvVar env("ASSET_ROOT", temp_root.string());
  const auto resolved = client::ResolveAssetPath("assets/sample.txt");

  EXPECT_EQ(resolved, file_path.string());

  std::filesystem::remove_all(temp_root);
}

TEST(AssetPathTest, ReturnsEmptyWhenAssetMissing) {
  const auto temp_root =
      std::filesystem::temp_directory_path() / "rtype_missing_assets";
  std::filesystem::create_directories(temp_root);

  ScopedEnvVar env("ASSET_ROOT", temp_root.string());
  const auto resolved =
      client::ResolveAssetPath("assets/does_not_exist_12345.txt");

  EXPECT_TRUE(resolved.empty());

  std::filesystem::remove_all(temp_root);
}
