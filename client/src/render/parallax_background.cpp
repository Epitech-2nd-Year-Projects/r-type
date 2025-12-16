#include "render/parallax_background.h"

#include <algorithm>
#include <cmath>

namespace client {

namespace {

constexpr float kBaseScrollSpeed = 120.0f;         // world units per second
constexpr float kScrollWrapDistance = 100000.0f;   // avoid float drift
constexpr float kMinimumHeightFraction = 0.05f;    // avoid zero scale
constexpr float kMinimumSpacingMultiplier = 1.0f;  // prevent overlap

}  // namespace

ParallaxBackground::Layer ParallaxBackground::MakeLayer(
    const std::shared_ptr<engine::render::Texture2D>& texture, float parallax,
    float speed_multiplier, float height_fraction, float spacing_multiplier,
    float anchor, engine::render::Color tint, bool flip_vertical) {
  ParallaxBackground::Layer layer{};
  layer.texture = texture;
  layer.parallax = parallax;
  layer.speed_multiplier = speed_multiplier;
  layer.height_fraction = height_fraction;
  layer.spacing_multiplier = spacing_multiplier;
  layer.anchor = anchor;
  layer.flip_vertical = flip_vertical;
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

  if (back) {
    layers_.push_back(
        MakeLayer(back, 0.25f, 0.6f, 1.0f, 1.0f, 0.0f,
                  engine::render::Color::FromBytes(240, 240, 255, 240), false));
  }
  if (stars) {
    layers_.push_back(MakeLayer(stars, 0.45f, 0.75f, 1.0f, 1.0f, 0.0f,
                                engine::render::Color::White(), false));
  }
  if (planet) {
    layers_.push_back(MakeLayer(planet, 0.9f, 1.05f, 0.55f, 1.5f, 1.0f,
                                engine::render::Color::White(), false));
    layers_.push_back(MakeLayer(planet, 0.9f, 1.05f, 0.55f, 1.5f, 0.0f,
                                engine::render::Color::White(), true));
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
    scroll_position_ = std::fmod(scroll_position_, kScrollWrapDistance);
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
  const float first_tile_index =
      std::floor((view.top_left_x_ + scroll) / spacing);
  float start = (first_tile_index - 1.0f) * spacing;
  const float end = view.top_left_x_ + view.width_ + scroll + spacing;

  const float anchor = std::clamp(layer.anchor, 0.0f, 1.0f);
  const float available_vertical = std::max(world_height - tile_height, 0.0f);
  const float y = available_vertical * anchor;

  engine::render::SpriteDrawParams params{};
  params.scale = {scale, layer.flip_vertical ? -scale : scale};
  params.layer = engine::render::RenderLayer::kBackground;
  params.tint = layer.tint;

  for (float x = start; x < end; x += spacing) {
    const float adjusted_y = layer.flip_vertical ? y + tile_height : y;
    const engine::math::Vector2f world_pos{x - scroll, adjusted_y};
    params.position = camera_.WorldToScreen(world_pos, layer.parallax);
    params.origin = layer.flip_vertical
                        ? engine::math::Vector2f{0.0f, tile_height}
                        : engine::math::Vector2f{0.0f, 0.0f};
    renderer_.DrawTexture(*layer.texture, params);
  }
}

}  // namespace client
