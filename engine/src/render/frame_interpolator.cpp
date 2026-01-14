#include "engine/render/frame_interpolator.h"

#include <algorithm>
#include <cmath>

namespace engine::render {

namespace {

float Lerp(float a, float b, float t) { return a + (b - a) * t; }

math::Vector2f LerpVector(const math::Vector2f& a, const math::Vector2f& b,
                          float t) {
  return {Lerp(a.x, b.x, t), Lerp(a.y, b.y, t)};
}

float LerpAngle(float a, float b, float t) {
  float diff = b - a;
  while (diff < -180.0f) diff += 360.0f;
  while (diff > 180.0f) diff -= 360.0f;
  return a + diff * t;
}

const SpriteData* FindSprite(const std::vector<SpriteData>& sprites,
                             std::uint32_t entity_id) {
  auto it = std::find_if(
      sprites.begin(), sprites.end(),
      [entity_id](const SpriteData& s) { return s.entity_id == entity_id; });
  if (it != sprites.end()) {
    return &(*it);
  }
  return nullptr;
}

}  // namespace

FrameInterpolator::FrameInterpolator(SnapshotBuffer& buffer)
    : buffer_(buffer) {}

std::vector<InterpolatedSprite> FrameInterpolator::Interpolate(
    std::uint64_t render_time_ns) {
  auto& buf = buffer_;
  auto [prev, curr] = buf.GetInterpolationPair();

  std::vector<InterpolatedSprite> result;
  result.reserve(curr.sprites.size());

  if (!prev.valid || !curr.valid) {
    return result;
  }
  float alpha = 0.0f;
  if (curr.timestamp_ns > prev.timestamp_ns) {
    double duration =
        static_cast<double>(curr.timestamp_ns - prev.timestamp_ns);
    double elapsed = static_cast<double>(render_time_ns) -
                     static_cast<double>(prev.timestamp_ns);
    alpha = static_cast<float>(elapsed / duration);
  }

  alpha = std::clamp(alpha, 0.0f, 1.0f);

  for (const auto& curr_sprite : curr.sprites) {
    if (!curr_sprite.visible) continue;

    InterpolatedSprite sprite;
    sprite.tint = curr_sprite.tint;
    sprite.source_rect = curr_sprite.source_rect;
    sprite.texture_path = curr_sprite.texture_path;
    sprite.origin = curr_sprite.origin;
    sprite.flip_x = curr_sprite.flip_x;
    sprite.flip_y = curr_sprite.flip_y;
    sprite.visible = curr_sprite.visible;

    const auto* prev_sprite = FindSprite(prev.sprites, curr_sprite.entity_id);

    if (prev_sprite) {
      sprite.position =
          LerpVector(prev_sprite->position, curr_sprite.position, alpha);
      sprite.rotation =
          LerpAngle(prev_sprite->rotation, curr_sprite.rotation, alpha);
      sprite.scale = LerpVector(prev_sprite->scale, curr_sprite.scale, alpha);
    } else {
      sprite.position = curr_sprite.position;
      sprite.rotation = curr_sprite.rotation;
      sprite.scale = curr_sprite.scale;
    }

    result.push_back(sprite);
  }

  return result;
}

}  // namespace engine::render
