#ifndef RIFT_ARENA_CONSTANTS_H_
#define RIFT_ARENA_CONSTANTS_H_

namespace rift {

/// Arena and fighter dimension constants shared between server and client.
struct ArenaConstants {
  static constexpr float kArenaWidth = 1280.0f;
  static constexpr float kFighterWidth = 60.0f;
  static constexpr float kFighterHeight = 120.0f;
  static constexpr float kGroundY = 380.0f;
  static constexpr float kMinDistance = 20.0f;
  static constexpr float kGravity = 980.0f;

  static constexpr float kFloorRenderY = kGroundY + kFighterHeight;

  static constexpr float kPlayer1SpawnX = 340.0f;
  static constexpr float kPlayer2SpawnX = 880.0f;
};

}  // namespace rift

#endif  // RIFT_ARENA_CONSTANTS_H_
