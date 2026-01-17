#ifndef RIFT_CLIENT_FIGHT_CAMERA_H_
#define RIFT_CLIENT_FIGHT_CAMERA_H_

#include "engine/render/camera3d.h"

namespace rift::client {

class FightCamera {
 public:
  FightCamera();

  void UpdateTarget(float fighter1_x, float fighter2_x);

  const engine::render::Camera3D& GetCamera() const { return camera_; }

 private:
  engine::render::Camera3D camera_;
  float height_offset_{1.5f};
  float fixed_distance_{12.0f};
};

}  // namespace rift::client

#endif  // RIFT_CLIENT_FIGHT_CAMERA_H_
