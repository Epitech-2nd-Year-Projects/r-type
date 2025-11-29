#ifndef ENGINE_RENDER_DRAW_LIST_H_
#define ENGINE_RENDER_DRAW_LIST_H_

#include <array>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "engine/math/vector2.h"
#include "engine/render/camera2d.h"
#include "engine/render/color.h"
#include "engine/render/layer.h"
#include "engine/render/renderer2d.h"
#include "engine/render/sprite.h"

namespace engine::render {

struct TextDrawParams {
  math::Vector2f position{};
  float font_size{16.0f};
  Color color{Color::White()};
  RenderLayer layer{RenderLayer::kMidground};
};

/**
 * @brief Batched draw commands to decouple gameplay code from Renderer2D.
 */
class DrawList {
 public:
  void Clear() noexcept;
  bool Empty() const noexcept;

  void AddSprite(const Sprite& sprite);
  void AddQuad(const std::shared_ptr<Texture2D>& texture,
               const SpriteDrawParams& params);
  void AddText(std::string_view text, const TextDrawParams& params);

  void Submit(Renderer2D& renderer, bool auto_clear = true);
  void Submit(Renderer2D& renderer, const Camera2D& camera,
              bool auto_clear = true);

 private:
  struct QuadCommand {
    std::shared_ptr<Texture2D> texture;
    SpriteDrawParams params;
  };

  struct TextCommand {
    std::string text;
    TextDrawParams params;
  };

  std::array<std::vector<QuadCommand>, kRenderLayerCount> quad_commands_;
  std::array<std::vector<TextCommand>, kRenderLayerCount> text_commands_;
};

}  // namespace engine::render

#endif  // ENGINE_RENDER_DRAW_LIST_H_
