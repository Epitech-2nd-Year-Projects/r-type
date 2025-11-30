#ifndef ENGINE_ENGINE_UTIL_CONFIG_H_
#define ENGINE_ENGINE_UTIL_CONFIG_H_

#include <filesystem>
#include <mutex>
#include <string>
#include <string_view>
#include <unordered_map>

namespace engine::util {

/**
 * @brief Runtime configuration shared across engine modules.
 */
class Configuration {
 public:
  Configuration() = default;

  /**
   * @brief Load key/value pairs from a .ini-style file.
   * @return false when the file could not be opened.
   */
  bool LoadFromFile(const std::filesystem::path& path,
                    bool override_existing = true);

  /**
   * @brief Load environment variables, optionally filtered by prefix.
   */
  void LoadFromEnvironment(const std::string& prefix = std::string{},
                           bool override_existing = true);

  /**
   * @brief Programmatically inject a value.
   */
  void Set(std::string key, std::string value, bool override_existing = true);

  /**
   * @brief Remove every stored key.
   */
  void Clear();

  /**
   * @brief Query helpers returning typed values.
   */
  [[nodiscard]] bool Has(std::string_view key) const;
  [[nodiscard]] std::string GetString(std::string_view key,
                                      std::string default_value = {}) const;
  [[nodiscard]] int GetInt(std::string_view key, int default_value = 0) const;
  [[nodiscard]] double GetDouble(std::string_view key,
                                 double default_value = 0.0) const;
  [[nodiscard]] float GetFloat(std::string_view key,
                               float default_value = 0.f) const;
  [[nodiscard]] bool GetBool(std::string_view key,
                             bool default_value = false) const;

  /**
   * @brief Snapshot current values for diagnostics/testing.
   */
  [[nodiscard]] std::unordered_map<std::string, std::string> Snapshot() const;

  /**
   * @brief Access the process-wide Configuration instance.
   */
  static Configuration& Instance();

 private:
  bool TryGetValue(std::string_view key, std::string* value) const;
  void SetInternal(std::string key, std::string value, bool override_existing);
  static std::string NormalizeKey(std::string_view key);
  static std::string TrimCopy(std::string_view text);
  static bool ParseBool(std::string_view text, bool fallback);

  std::unordered_map<std::string, std::string> values_;
  mutable std::mutex mutex_;
};

Configuration& GlobalConfig();

}  // namespace engine::util

#endif /* !ENGINE_ENGINE_UTIL_CONFIG_H_ */
