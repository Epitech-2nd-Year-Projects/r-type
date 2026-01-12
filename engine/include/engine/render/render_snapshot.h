#ifndef ENGINE_RENDER_RENDER_SNAPSHOT_H_
#define ENGINE_RENDER_RENDER_SNAPSHOT_H_

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "engine/ecs/registry.h"
#include "engine/math/rect.h"
#include "engine/math/vector2.h"

namespace engine::render {

struct SpriteData {
  std::uint32_t entity_id;
  math::Vector2f position;
  math::Vector2f origin;
  math::Vector2f scale;
  float rotation;
  math::RectF source_rect;
  struct Color {
    std::uint8_t r, g, b, a;
  } tint;
  std::uint8_t layer;
  std::string texture_path;
  bool visible;
  bool flip_x;
  bool flip_y;
};

struct RenderSnapshot {
  std::uint32_t tick;
  std::uint64_t timestamp_ns;
  std::vector<SpriteData> sprites;
  std::optional<math::Vector2f> camera_target;
  float parallax_offset;
  bool valid{false};
};

RenderSnapshot ExtractSnapshot(const ecs::Registry& registry,
                               std::uint32_t tick);

}  // namespace engine::render

#endif  // ENGINE_RENDER_RENDER_SNAPSHOT_H_
