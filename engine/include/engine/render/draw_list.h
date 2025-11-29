#ifndef ENGINE_RENDER_DRAW_LIST_H_
#define ENGINE_RENDER_DRAW_LIST_H_

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "engine/math/vector2.h"
#include "engine/render/color.h"
#include "engine/render/renderer2d.h"
#include "engine/render/sprite.h"

namespace engine::render {

struct TextDrawParams {
  math::Vector2f position{};
  float font_size{16.0f};
  Color color{Color::White()};
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

 private:
  struct QuadCommand {
    std::shared_ptr<Texture2D> texture;
    SpriteDrawParams params;
  };

  struct TextCommand {
    std::string text;
    TextDrawParams params;
  };

  std::vector<QuadCommand> quad_commands_;
  std::vector<TextCommand> text_commands_;
};

}  // namespace engine::render

#endif  // ENGINE_RENDER_DRAW_LIST_H_
