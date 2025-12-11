#ifndef CLIENT_SCENE_SETTINGS_SCENE_H_
#define CLIENT_SCENE_SETTINGS_SCENE_H_

#include <vector>
#include <memory>

#include "scene.h"
#include "../ui/ui_element.h"
#include "../ui/label.h"

namespace client {

class Application;

class SettingsScene : public Scene {
 public:
  explicit SettingsScene(Application& app);

  void Update(engine::time::TimeDelta dt) override;
  void Draw(engine::render::Renderer2D& renderer) override;

 private:
  Application& app_;
  std::vector<std::shared_ptr<ui::UIElement>> ui_elements_;
  
  std::shared_ptr<ui::Label> music_volume_label_;
  std::shared_ptr<ui::Label> sfx_volume_label_;
};

}  // namespace client

#endif  // CLIENT_SCENE_SETTINGS_SCENE_H_
