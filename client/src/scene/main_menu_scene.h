#ifndef CLIENT_SCENE_MAIN_MENU_SCENE_H_
#define CLIENT_SCENE_MAIN_MENU_SCENE_H_

#include <memory>
#include <vector>

#include "scene.h"
#include "../ui/button.h"
#include "../ui/text_input.h"
#include "../ui/ui_element.h"
#include "engine/ui/canvas.h"
#include "engine/ui/layouts.h"
#include "engine/ui/text.h"

namespace client {

class Application;

class MainMenuScene : public Scene {
 public:
  explicit MainMenuScene(Application& app);

  void Update(engine::time::TimeDelta dt) override;
  void Draw(engine::render::Renderer2D& renderer) override;

 private:
  void LayoutUi(engine::render::Renderer2D& renderer);

  Application& app_;
  std::vector<std::shared_ptr<ui::UIElement>> ui_elements_;
  
  std::shared_ptr<ui::TextInput> host_input_;
  std::shared_ptr<ui::TextInput> port_input_;
  std::shared_ptr<ui::TextInput> name_input_;
  std::shared_ptr<ui::Button> connect_button_;
  std::shared_ptr<ui::Button> settings_button_;
  engine::ui::Canvas canvas_;
  std::shared_ptr<engine::ui::TextElement> title_text_;
  
  bool was_mouse_down_{false};
};

}  // namespace client

#endif  // CLIENT_SCENE_MAIN_MENU_SCENE_H_
