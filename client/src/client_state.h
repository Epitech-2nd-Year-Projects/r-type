/**
 * @file client_state
 * @brief Client state types
 */

#ifndef CLIENT_CLIENT_STATE_H_
#define CLIENT_CLIENT_STATE_H_

#include <cstdint>

namespace client {

/**
 * @brief High level client state controlling active screen
 */
enum class ClientState {
  kSplash,
  kMainMenu,
  kLobby,
  kSettings,
  kAudioSettings,
  kConnecting,
  kInGame,
  kPaused,
  kGameOver,
  kDisconnected
};

/**
 * @brief Snapshot of end of match stats
 */
struct GameOverStats {
  std::uint32_t score{0};
  std::uint32_t wave{1};
};

}  // namespace client

#endif  // CLIENT_CLIENT_STATE_H_
