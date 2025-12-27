#ifndef CLIENT_SCENE_SETTINGS_SCENE_H_
#define CLIENT_SCENE_SETTINGS_SCENE_H_

#include <functional>
#include <memory>
#include <optional>
#include <vector>

#include "scene.h"
#include "engine/ui/canvas.h"
#include "engine/ui/layouts.h"
#include "engine/ui/text.h"
#include "engine/ui/button.h"
#include "../input_layer.h"
#include "engine/ui/types.h"

namespace client {

class ClientContext;
namespace ui {
class Button;
}

class SettingsScene : public Scene {
 public:
  explicit SettingsScene(ClientContext& context);

  void Update(engine::time::TimeDelta dt) override;
  void Draw(engine::render::Renderer2D& renderer) override;

 private:
  void LayoutUi(engine::render::Renderer2D& renderer);
  ClientContext& context_;

  engine::ui::Canvas canvas_;

  std::vector<std::shared_ptr<engine::ui::Button>> buttons_;

  std::shared_ptr<engine::ui::TextElement> music_volume_label_;
  std::shared_ptr<engine::ui::TextElement> sfx_volume_label_;
  std::shared_ptr<engine::ui::TextElement> rebind_status_label_;

  std::vector<bool> key_state_buffer_;
  std::optional<GameAction> pending_rebind_;

  struct BindingRow {
    GameAction action;
    std::shared_ptr<engine::ui::Button> button;

    BindingRow(GameAction action_in,
               std::shared_ptr<engine::ui::Button> button_in)
        : action(action_in), button(std::move(button_in)) {}
  };
  std::vector<BindingRow> binding_rows_;
  std::optional<std::reference_wrapper<BindingRow>> FindRow(GameAction action);
};

}  // namespace client

#endif  // CLIENT_SCENE_SETTINGS_SCENE_H_
