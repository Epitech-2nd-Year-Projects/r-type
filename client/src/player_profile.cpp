/**
 * @file player_profile.cpp
 * @brief Player profile persistence implementation
 */

#include "player_profile.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>

namespace client {

namespace {

constexpr const char kProfileFilename[] = "config/profile.json";

constexpr std::size_t kMaxNicknameLength = 16;

std::filesystem::path GetProfilePath() {
  return std::filesystem::path{kProfileFilename};
}

}  // namespace

PlayerProfile LoadPlayerProfile() {
  PlayerProfile profile;

  const auto path = GetProfilePath();
  std::ifstream file(path);
  if (!file.is_open()) {
    return profile;
  }

  nlohmann::json doc;
  try {
    file >> doc;
  } catch (const std::exception&) {
    return profile;
  }

  if (!doc.is_object()) {
    return profile;
  }

  if (doc.contains("nickname") && doc["nickname"].is_string()) {
    auto name = doc["nickname"].get<std::string>();
    if (!name.empty() && name.length() <= kMaxNicknameLength) {
      profile.nickname = std::move(name);
    }
  }

  if (doc.contains("stats") && doc["stats"].is_object()) {
    const auto& stats = doc["stats"];
    if (stats.contains("total_playtime_seconds") &&
        stats["total_playtime_seconds"].is_number_unsigned()) {
      profile.stats.total_playtime_seconds =
          stats["total_playtime_seconds"].get<std::uint64_t>();
    }
    if (stats.contains("total_deaths") &&
        stats["total_deaths"].is_number_unsigned()) {
      profile.stats.total_deaths = stats["total_deaths"].get<std::uint32_t>();
    }
    if (stats.contains("highest_score") &&
        stats["highest_score"].is_number_unsigned()) {
      profile.stats.highest_score = stats["highest_score"].get<std::uint32_t>();
    }
    if (stats.contains("games_played") &&
        stats["games_played"].is_number_unsigned()) {
      profile.stats.games_played = stats["games_played"].get<std::uint32_t>();
    }
  }

  if (doc.contains("avatar_index") &&
      doc["avatar_index"].is_number_unsigned()) {
    profile.avatar_index = doc["avatar_index"].get<std::uint8_t>();
  }

  if (doc.contains("chat_color_index") &&
      doc["chat_color_index"].is_number_unsigned()) {
    profile.chat_color_index = doc["chat_color_index"].get<std::uint8_t>();
  }

  return profile;
}

bool SavePlayerProfile(const PlayerProfile& profile) {
  const auto path = GetProfilePath();

  try {
    if (path.has_parent_path()) {
      std::filesystem::create_directories(path.parent_path());
    }
  } catch (const std::exception&) {
    return false;
  }

  nlohmann::json doc;
  doc["nickname"] = profile.nickname;
  doc["stats"]["total_playtime_seconds"] = profile.stats.total_playtime_seconds;
  doc["stats"]["total_deaths"] = profile.stats.total_deaths;
  doc["stats"]["highest_score"] = profile.stats.highest_score;
  doc["stats"]["games_played"] = profile.stats.games_played;
  doc["avatar_index"] = profile.avatar_index;
  doc["chat_color_index"] = profile.chat_color_index;

  std::ofstream file(path);
  if (!file.is_open()) {
    return false;
  }

  try {
    file << std::setw(2) << doc << std::endl;
  } catch (const std::exception&) {
    return false;
  }

  return file.good();
}

void UpdateProfileStats(PlayerProfile& profile,
                        std::uint64_t session_playtime_seconds,
                        std::uint32_t session_deaths,
                        std::uint32_t session_score) {
  profile.stats.total_playtime_seconds += session_playtime_seconds;
  profile.stats.total_deaths += session_deaths;
  profile.stats.games_played += 1;
  if (session_score > profile.stats.highest_score) {
    profile.stats.highest_score = session_score;
  }
}

}  // namespace client
