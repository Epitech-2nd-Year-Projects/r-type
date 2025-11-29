#include "engine/render/draw_list.h"

#include <utility>

#include "engine/render/layer.h"

namespace engine::render {

void DrawList::Clear() noexcept {
  for (auto& layer_commands : quad_commands_) {
    layer_commands.clear();
  }
  for (auto& text_layer : text_commands_) {
    text_layer.clear();
  }
}

bool DrawList::Empty() const noexcept {
  for (const auto& layer_commands : quad_commands_) {
    if (!layer_commands.empty()) {
      return false;
    }
  }
  for (const auto& text_layer : text_commands_) {
    if (!text_layer.empty()) {
      return false;
    }
  }
  return true;
}

void DrawList::AddSprite(const Sprite& sprite) {
  const auto& texture = sprite.GetTexture();
  if (!texture) {
    return;
  }

  QuadCommand cmd{};
  cmd.texture = texture;
  cmd.params = sprite.Params();
  quad_commands_[ToLayerIndex(cmd.params.layer)].push_back(std::move(cmd));
}

void DrawList::AddQuad(const std::shared_ptr<Texture2D>& texture,
                       const SpriteDrawParams& params) {
  if (!texture) {
    return;
  }
  QuadCommand cmd{texture, params};
  quad_commands_[ToLayerIndex(params.layer)].push_back(std::move(cmd));
}

void DrawList::AddText(std::string_view text, const TextDrawParams& params) {
  if (text.empty()) {
    return;
  }
  TextCommand cmd{std::string(text), params};
  text_commands_[ToLayerIndex(params.layer)].push_back(std::move(cmd));
}

namespace {

SpriteDrawParams ApplyCamera(const SpriteDrawParams& params,
                             const Camera2D& camera, RenderLayer layer) {
  SpriteDrawParams transformed = params;
  const float zoom = camera.GetZoom();
  transformed.position =
      camera.Apply(transformed.position, GetLayerParallax(layer));
  transformed.origin *= zoom;
  transformed.scale *= zoom;
  return transformed;
}

TextDrawParams ApplyCamera(const TextDrawParams& params, const Camera2D& camera,
                           RenderLayer layer) {
  TextDrawParams transformed = params;
  const float zoom = camera.GetZoom();
  transformed.position =
      camera.Apply(transformed.position, GetLayerParallax(layer));
  transformed.font_size *= zoom;
  return transformed;
}

}  // namespace

void DrawList::Submit(Renderer2D& renderer, bool auto_clear) {
  Camera2D default_camera;
  Submit(renderer, default_camera, auto_clear);
}

void DrawList::Submit(Renderer2D& renderer, const Camera2D& camera,
                      bool auto_clear) {
  for (RenderLayer layer : kRenderLayerDrawOrder) {
    const auto layer_index = ToLayerIndex(layer);
    for (const auto& quad : quad_commands_[layer_index]) {
      if (!quad.texture) {
        continue;
      }
      renderer.DrawTexture(*quad.texture,
                           ApplyCamera(quad.params, camera, layer));
    }

    for (const auto& text : text_commands_[layer_index]) {
      const auto params = ApplyCamera(text.params, camera, layer);
      renderer.DrawText(text.text, params.position, params.font_size,
                        params.color);
    }
  }

  renderer.Flush();

  if (auto_clear) {
    Clear();
  }
}

}  // namespace engine::render
