#ifndef RIFT_CLIENT_RIFT_STATE_H_
#define RIFT_CLIENT_RIFT_STATE_H_

#include <cstdint>

namespace rift::client {

enum class RiftClientState {
  kConnecting,
  kWaitingRoom,
  kPlaying,
  kMatchOver
};

struct MatchOverStats {
  std::uint32_t winner_player_id{0};
  std::uint8_t player1_rounds_won{0};
  std::uint8_t player2_rounds_won{0};
};

}  // namespace rift::client

#endif  // RIFT_CLIENT_RIFT_STATE_H_
