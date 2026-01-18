#include "fight_camera.h"

#include <algorithm>
#include <cmath>

namespace rift::client {

FightCamera::FightCamera() {
  camera_.SetYaw(0.0f);
  camera_.SetPitch(-10.0f);
  camera_.SetFov(45.0f);
  camera_.SetDistance(12.0f);
}

void FightCamera::UpdateTarget(float fighter1_x, float fighter2_x) {
  const float midpoint_x = (fighter1_x + fighter2_x) / 2.0f;

  camera_.SetDistance(fixed_distance_);
  camera_.SetTarget({midpoint_x, height_offset_, 0.0f});
}

}  // namespace rift::client
