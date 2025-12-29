#ifndef CLIENT_SCENE_MAIN_MENU_SCENE_H_
#define CLIENT_SCENE_MAIN_MENU_SCENE_H_

#include <memory>
#include <string>
#include <vector>

#include <raymedia.h>

#include "scene.h"
#include "ui/menu_effects.h"
#include "engine/ui/button.h"
#include "engine/ui/canvas.h"
#include "engine/ui/layouts.h"
#include "engine/ui/text.h"

namespace client {

class ClientContext;

class MainMenuScene : public Scene {
 public:
  explicit MainMenuScene(ClientContext& context);
  /**
   * @brief Release main menu video background resources
   */
  ~MainMenuScene() override;
  void Update(engine::time::TimeDelta dt) override;
  void Draw(engine::render::Renderer2D& renderer) override;

 private:
  void DrawBackground();
  void LayoutUi(engine::render::Renderer2D& renderer);
  void DrawTitle(engine::render::Renderer2D& renderer);
  void DrawVersion(engine::render::Renderer2D& renderer);

  ClientContext& context_;
  std::vector<std::shared_ptr<engine::ui::Button>> ui_elements_;
  std::shared_ptr<engine::ui::Button> play_button_;
  std::shared_ptr<engine::ui::Button> settings_button_;
  std::shared_ptr<engine::ui::Button> quit_button_;
  engine::ui::Canvas canvas_;
  std::shared_ptr<engine::render::Texture2D> title_texture_;
  engine::math::RectF title_rect_{};
  ui::MenuEffects menu_effects_;
  std::string version_text_;
  MediaStream background_media_{};
  bool background_media_loaded_{false};
};

}  // namespace client

#endif  // CLIENT_SCENE_MAIN_MENU_SCENE_H_
