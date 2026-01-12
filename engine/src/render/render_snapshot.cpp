#include "engine/render/render_snapshot.h"

#include <chrono>

#include "engine/ecs/components/sprite_component.h"
#include "engine/ecs/components/transform_component.h"
#include "engine/ecs/registry.h"

namespace engine::render {

RenderSnapshot ExtractSnapshot(const ecs::Registry& registry,
                               std::uint32_t tick) {
  RenderSnapshot snapshot;
  snapshot.tick = tick;
  snapshot.timestamp_ns = static_cast<std::uint64_t>(
      std::chrono::duration_cast<std::chrono::nanoseconds>(
          std::chrono::system_clock::now().time_since_epoch())
          .count());
  try {
    const auto& sprites = registry.GetComponents<ecs::SpriteComponent>();
    const auto& transforms = registry.GetComponents<ecs::TransformComponent>();

    for (std::size_t i = 0; i < sprites.size(); ++i) {
      if (!sprites[i].has_value() || !transforms[i].has_value()) {
        continue;
      }

      const auto& sprite = sprites[i].value();
      const auto& transform = transforms[i].value();

      if (!sprite.visible) {
        continue;
      }

      SpriteData data;
      data.entity_id = static_cast<std::uint32_t>(i);
      data.position = transform.transform.GetPosition();
      data.rotation = transform.transform.GetRotation();
      data.scale = transform.transform.GetScale();
      data.source_rect = sprite.source_rect;
      data.tint = {sprite.tint.r, sprite.tint.g, sprite.tint.b, sprite.tint.a};
      data.layer = sprite.layer;
      data.texture_path = sprite.texture_path;
      data.visible = sprite.visible;
      data.flip_x = sprite.flip_x;
      data.flip_y = sprite.flip_y;
      data.origin = {0.0f, 0.0f};

      snapshot.sprites.push_back(std::move(data));
    }
  } catch (const std::exception&) {
  }

  snapshot.valid = true;
  return snapshot;
}

}  // namespace engine::render
