#ifndef CLIENT_SCENE_SETTINGS_SCENE_H_
#define CLIENT_SCENE_SETTINGS_SCENE_H_

#include <vector>
#include <memory>
#include <optional>

#include "scene.h"
#include "../ui/ui_element.h"
#include "../ui/label.h"
#include "../ui/button.h"
#include "../input_layer.h"

namespace client {

class Application;
namespace ui {
class Button;
}

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

    BindingRow(GameAction action_in, std::shared_ptr<ui::Label> label_in,
               std::shared_ptr<ui::Button> button_in)
        : action(action_in), label(std::move(label_in)),
          button(std::move(button_in)) {}
  };
  std::vector<BindingRow> binding_rows_;
  BindingRow* FindRow(GameAction action);
};

}  // namespace client

#endif  // CLIENT_SCENE_SETTINGS_SCENE_H_
