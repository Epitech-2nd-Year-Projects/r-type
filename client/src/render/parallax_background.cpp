#include "render/parallax_background.h"

#include <algorithm>
#include <cmath>
#include <cstdint>

#include "engine/util/logging.h"

namespace client {

namespace {

constexpr float kBaseScrollSpeed = 120.0f;
constexpr float kScrollWrapDistance = 100000.0f;
constexpr float kMinimumHeightFraction = 0.05f;
constexpr float kMinimumSpacingMultiplier = 1.0f;
constexpr int kTileBuffer = 2;

bool UseAlternateTile(int tile_index) {
  std::uint32_t value = static_cast<std::uint32_t>(tile_index);
  value ^= value << 13;
  value ^= value >> 17;
  value ^= value << 5;
  return (value & 1u) == 1u;
}

}  // namespace

ParallaxBackground::Layer ParallaxBackground::MakeLayer(
    const std::shared_ptr<engine::render::Texture2D>& texture, float parallax,
    float speed_multiplier, float height_fraction, float spacing_multiplier,
    float anchor, engine::render::Color tint, bool flip_vertical,
    bool randomize_vertical, float alternate_anchor,
    bool alternate_flip_vertical) {
  ParallaxBackground::Layer layer{};
  layer.texture = texture;
  layer.parallax = parallax;
  layer.speed_multiplier = speed_multiplier;
  layer.height_fraction = height_fraction;
  layer.spacing_multiplier = spacing_multiplier;
  layer.anchor = anchor;
  layer.alternate_anchor = alternate_anchor;
  layer.flip_vertical = flip_vertical;
  layer.alternate_flip_vertical = alternate_flip_vertical;
  layer.randomize_vertical = randomize_vertical;
  layer.tint = tint;
  return layer;
}

ParallaxBackground::ParallaxBackground(engine::render::Renderer2D& renderer)
    : renderer_(renderer) {
  const auto stars =
      renderer_.LoadTextureFromFile("assets/layered/bg-stars.png");
  const auto back = renderer_.LoadTextureFromFile("assets/layered/bg-back.png");
  const auto planet =
      renderer_.LoadTextureFromFile("assets/layered/bg-planet.png");

  auto& logger = engine::util::Logger::Default();
  if (!back) {
    logger.Warn(
        "ParallaxBackground: failed to load assets/layered/bg-back.png");
  }
  if (back) {
    layers_.push_back(
        MakeLayer(back, 0.25f, 0.6f, 1.0f, 1.0f, 0.0f,
                  engine::render::Color::FromBytes(240, 240, 255, 240), false));
  }
  if (!stars) {
    logger.Warn(
        "ParallaxBackground: failed to load assets/layered/bg-stars.png");
  }
  if (stars) {
    layers_.push_back(MakeLayer(stars, 0.45f, 0.75f, 1.0f, 1.0f, 0.0f,
                                engine::render::Color::White(), false));
  }
  if (!planet) {
    logger.Warn(
        "ParallaxBackground: failed to load assets/layered/bg-planet.png");
  }
  if (planet) {
    layers_.push_back(MakeLayer(planet, 0.9f, 1.05f, 0.55f, 1.35f, 1.0f,
                                engine::render::Color::White(), false, true,
                                0.0f, true));
  }
}

void ParallaxBackground::Update(engine::time::TimeDelta dt,
                                const engine::math::Vector2f& viewport_size) {
  if (viewport_size.x <= 0.0f || viewport_size.y <= 0.0f) {
    return;
  }

  camera_.SetViewportSize(viewport_size);
  camera_.SetVerticalRange(0.0f, viewport_size.y);

  scroll_position_ += kBaseScrollSpeed * dt.as_seconds();
  if (scroll_position_ > kScrollWrapDistance) {
    scroll_position_ -= kScrollWrapDistance;
  }

  camera_.SetFocusX(scroll_position_);
}

void ParallaxBackground::Draw() {
  if (layers_.empty()) {
    return;
  }

  const float world_height =
      camera_.GetVerticalMax() - camera_.GetVerticalMin();
  for (const auto& layer : layers_) {
    DrawLayer(layer, world_height);
  }
}

void ParallaxBackground::DrawLayer(const Layer& layer, float world_height) {
  if (!layer.texture) {
    return;
  }

  const auto size = layer.texture->GetSize();
  if (size.x == 0 || size.y == 0) {
    return;
  }

  const float clamped_height_fraction =
      std::max(layer.height_fraction, kMinimumHeightFraction);
  const float target_height = world_height * clamped_height_fraction;
  const float scale = target_height / static_cast<float>(size.y);
  const float tile_width = static_cast<float>(size.x) * scale;
  const float tile_height = target_height;
  const float spacing = tile_width * std::max(layer.spacing_multiplier,
                                              kMinimumSpacingMultiplier);
  const float scroll = scroll_position_ * layer.speed_multiplier;

  const auto view = camera_.GetViewRectWorld();
  const float view_left = view.top_left_x_ * layer.parallax;
  const int start_tile_index = static_cast<int>(
      std::floor((view_left + scroll) / spacing) - kTileBuffer);
  const int end_tile_index =
      static_cast<int>(
          std::floor((view_left + view.width_ + scroll) / spacing)) +
      kTileBuffer;

  const float available_vertical = std::max(world_height - tile_height, 0.0f);

  engine::render::SpriteDrawParams params{};
  params.scale = {scale, scale};
  params.layer = engine::render::RenderLayer::kBackground;
  params.tint = layer.tint;

  for (int tile = start_tile_index; tile <= end_tile_index; ++tile) {
    const bool use_alternate =
        layer.randomize_vertical ? UseAlternateTile(tile) : false;
    const bool flip_vertical =
        use_alternate ? layer.alternate_flip_vertical : layer.flip_vertical;
    const float anchor = std::clamp(
        use_alternate ? layer.alternate_anchor : layer.anchor, 0.0f, 1.0f);

    const float y = available_vertical * anchor;
    const float x = static_cast<float>(tile) * spacing;

    const engine::math::RectF source_rect{
        0.0f, flip_vertical ? static_cast<float>(size.y) : 0.0f,
        static_cast<float>(size.x),
        flip_vertical ? -static_cast<float>(size.y)
                      : static_cast<float>(size.y)};

    params.position = camera_.WorldToScreen({x - scroll, y}, layer.parallax);
    params.origin = {0.0f, 0.0f};
    params.source = source_rect;
    renderer_.DrawTexture(*layer.texture, params);
  }
}

}  // namespace client
