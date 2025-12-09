#include "engine/util/config.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <sstream>
#include <utility>
#include <vector>

namespace engine::util {

namespace {

constexpr std::string_view kWhitespace = " \t\r\n";

std::vector<std::string> ReadEnvironmentEntries() {
  std::ifstream env_stream("/proc/self/environ", std::ios::binary);
  if (!env_stream) {
    return {};
  }

  std::string content((std::istreambuf_iterator<char>(env_stream)),
                      std::istreambuf_iterator<char>());
  std::vector<std::string> entries;
  std::size_t start = 0;
  while (start < content.size()) {
    const auto end = content.find('\0', start);
    const auto length =
        end == std::string::npos ? content.size() - start : end - start;
    if (length > 0) {
      entries.emplace_back(content.substr(start, length));
    }
    if (end == std::string::npos) {
      break;
    }
    start = end + 1;
  }
  return entries;
}

}  // namespace

bool Configuration::LoadFromFile(const std::filesystem::path& path,
                                 bool override_existing) {
  std::ifstream file(path);
  if (!file.is_open()) {
    return false;
  }

  std::string line;
  while (std::getline(file, line)) {
    const auto trimmed = TrimCopy(line);
    if (trimmed.empty() || trimmed[0] == '#') continue;

    const auto delimiter_pos = trimmed.find('=');
    if (delimiter_pos == std::string::npos) continue;

    auto key = TrimCopy(trimmed.substr(0, delimiter_pos));
    auto value = TrimCopy(trimmed.substr(delimiter_pos + 1));
    if (!key.empty()) {
      if (value.size() >= 2 &&
          ((value.front() == '"' && value.back() == '"') ||
           (value.front() == '\'' && value.back() == '\''))) {
        value = value.substr(1, value.size() - 2);
      }
      SetInternal(NormalizeKey(key), value, override_existing);
    }
  }
  return true;
}

void Configuration::LoadFromEnvironment(const std::string& prefix,
                                        bool override_existing) {
  const auto entries = ReadEnvironmentEntries();
  if (entries.empty()) return;

  const auto normalized_prefix = NormalizeKey(prefix);

  for (const auto& entry : entries) {
    const auto pos = entry.find('=');
    if (pos == std::string::npos) continue;

    auto key = NormalizeKey(entry.substr(0, pos));
    if (!normalized_prefix.empty()) {
      if (key.rfind(normalized_prefix, 0) != 0) {
        continue;
      }
      key.erase(0, normalized_prefix.size());
      if (!key.empty() && key.front() == '.') {
        key.erase(0, 1);
      }
    }

    auto value = TrimCopy(entry.substr(pos + 1));
    if (!key.empty()) {
      SetInternal(std::move(key), std::move(value), override_existing);
    }
  }
}

void Configuration::Set(std::string key, std::string value,
                        bool override_existing) {
  SetInternal(NormalizeKey(key), std::move(value), override_existing);
}

void Configuration::Clear() {
  std::lock_guard lock(mutex_);
  values_.clear();
}

bool Configuration::Has(std::string_view key) const {
  std::lock_guard lock(mutex_);
  return values_.find(NormalizeKey(key)) != values_.end();
}

std::string Configuration::GetString(std::string_view key,
                                     std::string default_value) const {
  std::string value;
  if (!TryGetValue(key, value)) return default_value;
  return value;
}

int Configuration::GetInt(std::string_view key, int default_value) const {
  std::string value;
  if (!TryGetValue(key, value)) return default_value;
  try {
    return std::stoi(value);
  } catch (...) {
    return default_value;
  }
}

double Configuration::GetDouble(std::string_view key,
                                double default_value) const {
  std::string value;
  if (!TryGetValue(key, value)) return default_value;
  try {
    return std::stod(value);
  } catch (...) {
    return default_value;
  }
}

float Configuration::GetFloat(std::string_view key, float default_value) const {
  std::string value;
  if (!TryGetValue(key, value)) return default_value;
  try {
    return std::stof(value);
  } catch (...) {
    return default_value;
  }
}

bool Configuration::GetBool(std::string_view key, bool default_value) const {
  std::string value;
  if (!TryGetValue(key, value)) return default_value;
  return ParseBool(value, default_value);
}

std::unordered_map<std::string, std::string> Configuration::Snapshot() const {
  std::lock_guard lock(mutex_);
  return values_;
}

Configuration& Configuration::Instance() {
  static Configuration config;
  return config;
}

Configuration& GlobalConfig() { return Configuration::Instance(); }

bool Configuration::TryGetValue(std::string_view key,
                                std::string& value) const {
  const auto normalized = NormalizeKey(key);
  std::lock_guard lock(mutex_);
  auto it = values_.find(normalized);
  if (it == values_.end()) return false;
  value = it->second;
  return true;
}

void Configuration::SetInternal(std::string key, std::string value,
                                bool override_existing) {
  if (key.empty()) return;
  std::lock_guard lock(mutex_);
  auto it = values_.find(key);
  if (it == values_.end()) {
    values_.emplace(std::move(key), std::move(value));
  } else if (override_existing) {
    it->second = std::move(value);
  }
}

std::string Configuration::NormalizeKey(std::string_view key) {
  auto trimmed = TrimCopy(key);
  std::string normalized;
  normalized.reserve(trimmed.size());
  for (unsigned char c : trimmed) {
    if (kWhitespace.find(c) != std::string_view::npos) continue;
    if (c == '_' || c == '-') {
      normalized.push_back('.');
    } else {
      normalized.push_back(
          static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
    }
  }

  while (!normalized.empty() && normalized.front() == '.') {
    normalized.erase(normalized.begin());
  }
  while (!normalized.empty() && normalized.back() == '.') {
    normalized.pop_back();
  }
  return normalized;
}

std::string Configuration::TrimCopy(std::string_view text) {
  const auto begin = text.find_first_not_of(kWhitespace);
  if (begin == std::string_view::npos) return {};
  const auto end = text.find_last_not_of(kWhitespace);
  return std::string(text.substr(begin, end - begin + 1));
}

bool Configuration::ParseBool(std::string_view text, bool fallback) {
  std::string normalized(text.size(), '\0');
  std::transform(
      text.begin(), text.end(), normalized.begin(),
      [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  if (normalized == "1" || normalized == "true" || normalized == "yes" ||
      normalized == "on") {
    return true;
  }
  if (normalized == "0" || normalized == "false" || normalized == "no" ||
      normalized == "off") {
    return false;
  }
  return fallback;
}

}  // namespace engine::util
