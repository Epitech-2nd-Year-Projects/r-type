#ifndef RIFT_CLIENT_SCENE_SCENE_H_
#define RIFT_CLIENT_SCENE_SCENE_H_

#include "engine/render/renderer2d.h"
#include "engine/time/time_delta.h"

namespace rift::client {

class Scene {
 public:
  virtual ~Scene() = default;

  virtual void Update(engine::time::TimeDelta dt) = 0;

  virtual void Draw(engine::render::Renderer2D& renderer) = 0;

  virtual void DrawBackground(engine::render::Renderer2D& renderer) {}

  virtual void DrawForeground(engine::render::Renderer2D& renderer) {
    Draw(renderer);
  }
};

}  // namespace rift::client

#endif  // RIFT_CLIENT_SCENE_SCENE_H_
