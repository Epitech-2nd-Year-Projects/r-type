/**
 * @file player_profile.h
 * @brief Player profile data and persistence
 */

#ifndef CLIENT_PLAYER_PROFILE_H_
#define CLIENT_PLAYER_PROFILE_H_

#include <cstdint>
#include <string>

namespace client {

/**
 * @brief Local player statistics
 */
struct PlayerStats {
  std::uint64_t total_playtime_seconds{0};
  std::uint32_t total_deaths{0};
  std::uint32_t highest_score{0};
  std::uint32_t games_played{0};
};

/**
 * @brief Player profile with nickname and statistics
 */
struct PlayerProfile {
  std::string nickname{"Pilot"};
  PlayerStats stats{};
  std::uint8_t avatar_index{0};
};

/**
 * @brief Load player profile from disk
 * @return Loaded profile or default if file missing
 */
PlayerProfile LoadPlayerProfile();

/**
 * @brief Save player profile to disk
 * @param profile Profile to persist
 * @return True on success
 */
bool SavePlayerProfile(const PlayerProfile& profile);

/**
 * @brief Update profile stats after a game session
 * @param profile Profile to update
 * @param session_playtime_seconds Duration of the session
 * @param session_deaths Deaths during the session
 * @param session_score Score achieved in the session
 */
void UpdateProfileStats(PlayerProfile& profile,
                        std::uint64_t session_playtime_seconds,
                        std::uint32_t session_deaths,
                        std::uint32_t session_score);

}  // namespace client

#endif  // CLIENT_PLAYER_PROFILE_H_
