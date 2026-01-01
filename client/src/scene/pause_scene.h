#ifndef CLIENT_SCENE_PAUSE_SCENE_H_
#define CLIENT_SCENE_PAUSE_SCENE_H_

#include <memory>
#include <vector>

#include "scene.h"
#include "engine/math/rect.h"
#include "engine/ui/canvas.h"
#include "engine/ui/layouts.h"
#include "engine/ui/button.h"
#include "ui/menu_effects.h"

namespace client {

class ClientContext;

class PauseScene : public Scene {
 public:
  explicit PauseScene(ClientContext& context);

  void Update(engine::time::TimeDelta dt) override;
  void Draw(engine::render::Renderer2D& renderer) override;

 private:
  /**
   * @brief Build the pause menu widget tree
   */
  void BuildUi();
  /**
   * @brief Apply layout so widgets match the current window size
   */
  void LayoutUi(engine::render::Renderer2D& renderer);

  ClientContext& context_;
  ui::MenuEffects menu_effects_;
  engine::ui::Canvas canvas_;
  std::shared_ptr<engine::ui::Button> resume_button_;
  std::shared_ptr<engine::ui::Button> options_button_;
  std::shared_ptr<engine::ui::Button> quit_button_;
  std::vector<std::shared_ptr<engine::ui::Button>> menu_buttons_;
  std::vector<std::shared_ptr<engine::render::Texture2D>> top_frames_;
  std::vector<std::shared_ptr<engine::render::Texture2D>> bottom_frames_;
  engine::math::RectF top_rect_{};
  engine::math::RectF bottom_rect_{};
  float top_elapsed_{0.0f};
  float bottom_elapsed_{0.0f};
  bool pause_toggle_pressed_{false};
  bool confirm_pressed_{false};
};

}  // namespace client

#endif  // CLIENT_SCENE_PAUSE_SCENE_H_
