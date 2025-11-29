#include "engine/render/draw_list.h"

#include <utility>

namespace engine::render {

void DrawList::Clear() noexcept {
  quad_commands_.clear();
  text_commands_.clear();
}

bool DrawList::Empty() const noexcept {
  return quad_commands_.empty() && text_commands_.empty();
}

void DrawList::AddSprite(const Sprite& sprite) {
  const auto& texture = sprite.GetTexture();
  if (!texture) {
    return;
  }

  QuadCommand cmd{};
  cmd.texture = texture;
  cmd.params = sprite.Params();
  quad_commands_.push_back(std::move(cmd));
}

void DrawList::AddQuad(const std::shared_ptr<Texture2D>& texture,
                       const SpriteDrawParams& params) {
  if (!texture) {
    return;
  }
  quad_commands_.push_back(QuadCommand{texture, params});
}

void DrawList::AddText(std::string_view text, const TextDrawParams& params) {
  if (text.empty()) {
    return;
  }
  text_commands_.push_back(TextCommand{std::string(text), params});
}

void DrawList::Submit(Renderer2D& renderer, bool auto_clear) {
  for (const auto& quad : quad_commands_) {
    if (!quad.texture) {
      continue;
    }
    renderer.DrawTexture(*quad.texture, quad.params);
  }

  for (const auto& text : text_commands_) {
    renderer.DrawText(text.text, text.params.position, text.params.font_size,
                      text.params.color);
  }

  renderer.Flush();

  if (auto_clear) {
    Clear();
  }
}

}  // namespace engine::render
