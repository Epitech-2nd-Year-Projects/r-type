#ifndef CLIENT_SCENE_SETTINGS_SCENE_H_
#define CLIENT_SCENE_SETTINGS_SCENE_H_

#include <vector>
#include <memory>
#include <optional>

#include "scene.h"
#include "../ui/ui_element.h"
#include "../ui/label.h"
#include "../input_layer.h"

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
  std::vector<bool> key_state_buffer_;
  std::optional<GameAction> pending_rebind_;
  std::shared_ptr<ui::Label> rebind_status_label_;
  struct BindingRow {
    GameAction action;
    std::shared_ptr<ui::Label> label;
    std::shared_ptr<ui::Button> button;
  };
  std::vector<BindingRow> binding_rows_;
};

}  // namespace client

#endif  // CLIENT_SCENE_SETTINGS_SCENE_H_
