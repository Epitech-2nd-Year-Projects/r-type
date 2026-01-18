#ifndef CLIENT_SCENE_KEY_BINDINGS_SCENE_H_
#define CLIENT_SCENE_KEY_BINDINGS_SCENE_H_

#include <memory>
#include <string>
#include <vector>

#include "engine/render/renderer2d.h"
#include "engine/ui/button.h"
#include "engine/ui/canvas.h"
#include "input/key_bindings.h"
#include "scene.h"
#include "ui/menu_effects.h"

namespace client {

class ClientContext;

class KeyBindingsScene : public Scene {
 public:
  explicit KeyBindingsScene(ClientContext& context);

  void Update(engine::time::TimeDelta dt) override;
  void Draw(engine::render::Renderer2D& renderer) override;
  void DrawBackground(engine::render::Renderer2D& renderer) override;
  void DrawForeground(engine::render::Renderer2D& renderer) override;

 private:
  struct BindingRow {
    GameAction action;
    std::string label;
    std::shared_ptr<engine::ui::Button> button;
    engine::math::RectF row_rect{};
  };

  void LayoutUi(engine::render::Renderer2D& renderer);
  void DrawWarning(engine::render::Renderer2D& renderer);
  void DrawRows(engine::render::Renderer2D& renderer);
  void HandleRebind(GameAction action);
  void RefreshButtons();

  ClientContext& context_;
  ui::MenuEffects menu_effects_;
  engine::ui::Canvas canvas_;

  std::vector<std::shared_ptr<engine::render::Texture2D>> warning_frames_;
  float warning_elapsed_{0.0f};
  bool warning_animating_{true};
  engine::math::RectF warning_rect_{};

  std::vector<BindingRow> rows_;
  std::vector<std::shared_ptr<engine::ui::Button>> controls_;
  std::vector<std::shared_ptr<engine::ui::Button>> pointer_buttons_;
  std::shared_ptr<engine::ui::Button> back_button_;

  bool is_binding_{false};
  GameAction binding_action_{};
};

}  // namespace client

#endif  // CLIENT_SCENE_KEY_BINDINGS_SCENE_H_
