#ifndef RIFT_CLIENT_ARENA_3D_CONSTANTS_H_
#define RIFT_CLIENT_ARENA_3D_CONSTANTS_H_

#include "rift/arena_constants.h"

namespace rift::client {

/// 3D arena configuration derived from 2D constants.
struct Arena3DConstants {
  static constexpr float kScaleFactor = 0.0125f;

  static constexpr float kFloorY = 0.0f;

  static constexpr float kFighterBaseY = 0.0f;

  static constexpr float kArenaWidth3D =
      rift::ArenaConstants::kArenaWidth * kScaleFactor;
  static constexpr float kArenaDepth3D = 6.0f;

  static constexpr float kFighterModelScale = 1.5f;

  static constexpr float kPlayer1SpawnX3D =
      (rift::ArenaConstants::kPlayer1SpawnX -
       rift::ArenaConstants::kArenaWidth / 2.0f) *
      kScaleFactor;
  static constexpr float kPlayer2SpawnX3D =
      (rift::ArenaConstants::kPlayer2SpawnX -
       rift::ArenaConstants::kArenaWidth / 2.0f) *
      kScaleFactor;

  static constexpr float To3DX(float x_2d) {
    return (x_2d - rift::ArenaConstants::kArenaWidth / 2.0f) * kScaleFactor;
  }
};

}  // namespace rift::client

#endif  // RIFT_CLIENT_ARENA_3D_CONSTANTS_H_
