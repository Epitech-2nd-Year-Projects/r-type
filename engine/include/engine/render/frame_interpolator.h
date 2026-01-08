#ifndef ENGINE_RENDER_FRAME_INTERPOLATOR_H_
#define ENGINE_RENDER_FRAME_INTERPOLATOR_H_

#include <cstdint>
#include <string>
#include <vector>

#include "engine/math/rect.h"
#include "engine/math/vector2.h"
#include "engine/render/render_snapshot.h"
#include "engine/render/snapshot_buffer.h"

namespace engine::render {

struct InterpolatedSprite {
  math::Vector2f position;
  float rotation;
  math::Vector2f scale;

  SpriteData::Color tint;
  math::RectF source_rect;
  std::string texture_path;
  math::Vector2f origin;
  bool flip_x;
  bool flip_y;
  bool visible;
};

class FrameInterpolator {
 public:
  explicit FrameInterpolator(SnapshotBuffer& buffer);

  std::vector<InterpolatedSprite> Interpolate(std::uint64_t render_time_ns);

 private:
  SnapshotBuffer& buffer_;
};

}  // namespace engine::render

#endif  // ENGINE_RENDER_FRAME_INTERPOLATOR_H_
