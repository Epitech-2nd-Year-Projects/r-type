#ifndef CLIENT_SCENE_MAIN_MENU_SCENE_H_
#define CLIENT_SCENE_MAIN_MENU_SCENE_H_

#include <vector>
#include <memory>

#include "scene.h"
#include "../ui/ui_element.h"
#include "../ui/text_input.h"
#include "../ui/button.h"
#include "../ui/label.h"

namespace client {

class Application;

class MainMenuScene : public Scene {
 public:
  explicit MainMenuScene(Application& app);

  void Update(engine::time::TimeDelta dt) override;
  void Draw(engine::render::Renderer2D& renderer) override;

 private:
  Application& app_;
  std::vector<std::shared_ptr<ui::UIElement>> ui_elements_;
  
  std::shared_ptr<ui::TextInput> host_input_;
  std::shared_ptr<ui::TextInput> port_input_;
  std::shared_ptr<ui::TextInput> name_input_;
  
  bool was_mouse_down_{false};
};

}  // namespace client

#endif  // CLIENT_SCENE_MAIN_MENU_SCENE_H_