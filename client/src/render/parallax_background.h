#ifndef CLIENT_RENDER_PARALLAX_BACKGROUND_H_
#define CLIENT_RENDER_PARALLAX_BACKGROUND_H_

#include <memory>
#include <vector>

#include "engine/math/vector2.h"
#include "engine/render/camera25d.h"
#include "engine/render/color.h"
#include "engine/render/renderer2d.h"
#include "engine/time/time_delta.h"

namespace client {

/**
 * @brief Animated parallax background composed of layered textures.
 *
 * Uses Camera25D to transform world-aligned quads into screen space with
 * per-layer parallax factors. Layers scroll horizontally based on engine time
 * to keep motion frame-rate independent.
 */
class ParallaxBackground {
 public:
  explicit ParallaxBackground(engine::render::Renderer2D& renderer);

  /**
   * @brief Advance background animation and refresh viewport.
   * @param dt Frame delta time used for scroll speed.
   * @param viewport_size Current window size in pixels.
   */
  void Update(engine::time::TimeDelta dt,
              const engine::math::Vector2f& viewport_size);

  /**
   * @brief Draw all layers in back-to-front order.
   */
  void Draw();

 private:
  struct Layer {
    std::shared_ptr<engine::render::Texture2D> texture;
    float parallax{1.0f};
    float speed_multiplier{1.0f};
    float height_fraction{1.0f};
    float spacing_multiplier{1.0f};
    float anchor{0.0f};
    float alternate_anchor{0.0f};
    bool flip_vertical{false};
    bool alternate_flip_vertical{false};
    bool randomize_vertical{false};
    engine::render::Color tint{engine::render::Color::White()};
  };

  static Layer MakeLayer(
      const std::shared_ptr<engine::render::Texture2D>& texture,
      float parallax, float speed_multiplier, float height_fraction,
      float spacing_multiplier, float anchor,
      engine::render::Color tint = engine::render::Color::White(),
      bool flip_vertical = false, bool randomize_vertical = false,
      float alternate_anchor = 0.0f, bool alternate_flip_vertical = false);
  void DrawLayer(const Layer& layer, float world_height);

  engine::render::Renderer2D& renderer_;
  engine::render::Camera25D camera_{};
  std::vector<Layer> layers_;
  float scroll_position_{0.0f};
};

}  // namespace client

#endif  // CLIENT_RENDER_PARALLAX_BACKGROUND_H_
