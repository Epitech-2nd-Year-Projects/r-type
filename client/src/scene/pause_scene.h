#ifndef CLIENT_SCENE_PAUSE_SCENE_H_
#define CLIENT_SCENE_PAUSE_SCENE_H_

#include <memory>
#include <vector>

#include "scene.h"
#include "engine/ui/canvas.h"
#include "engine/ui/layouts.h"
#include "engine/ui/text.h"
#include "engine/ui/button.h"

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
  engine::ui::Canvas canvas_;
  std::shared_ptr<engine::ui::TextElement> title_;
  std::shared_ptr<engine::ui::Button> resume_button_;
  std::shared_ptr<engine::ui::Button> options_button_;
  std::shared_ptr<engine::ui::Button> quit_button_;
  std::vector<std::shared_ptr<engine::ui::Button>> menu_buttons_;
  bool pause_toggle_pressed_{false};
  bool confirm_pressed_{false};
};

}  // namespace client

#endif  // CLIENT_SCENE_PAUSE_SCENE_H_
