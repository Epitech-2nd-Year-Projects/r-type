#ifndef CLIENT_SCENE_MAIN_MENU_SCENE_H_
#define CLIENT_SCENE_MAIN_MENU_SCENE_H_

#include <array>
#include <memory>
#include <string>
#include <vector>

#include "scene.h"
#include "../ui/button.h"
#include "../ui/ui_element.h"
#include "engine/ui/canvas.h"
#include "engine/ui/layouts.h"
#include "engine/ui/text.h"

namespace client {

class ClientContext;

class MainMenuScene : public Scene {
 public:
  explicit MainMenuScene(ClientContext& context);
  void Update(engine::time::TimeDelta dt) override;
  void Draw(engine::render::Renderer2D& renderer) override;

 private:
  void LayoutUi(engine::render::Renderer2D& renderer);
  void DrawPointers(engine::render::Renderer2D& renderer);
  void DrawTitle(engine::render::Renderer2D& renderer);

  ClientContext& context_;
  std::vector<std::shared_ptr<ui::UIElement>> ui_elements_;
  std::shared_ptr<ui::Button> play_button_;
  std::shared_ptr<ui::Button> settings_button_;
  std::shared_ptr<ui::Button> quit_button_;
  engine::ui::Canvas canvas_;
  std::shared_ptr<engine::render::Texture2D> title_texture_;
  engine::math::RectF title_rect_{};
  std::vector<std::shared_ptr<engine::render::Texture2D>> pointer_frames_;
  std::string hover_sfx_path_;
  std::string click_sfx_path_;
  struct PointerState {
    bool hovered{false};
    bool was_hovered{false};
    float elapsed{0.0f};
    bool animating{false};
  };
  std::array<PointerState, 3> pointer_states_{};
};

}  // namespace client

#endif  // CLIENT_SCENE_MAIN_MENU_SCENE_H_
