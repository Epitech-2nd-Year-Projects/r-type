#ifndef CLIENT_SCENE_PAUSE_SCENE_H_
#define CLIENT_SCENE_PAUSE_SCENE_H_

#include <memory>
#include <vector>

#include "scene.h"
#include "engine/ui/canvas.h"
#include "engine/ui/layouts.h"
#include "engine/ui/text.h"
#include "ui/button.h"

namespace client {

class Application;

class PauseScene : public Scene {
 public:
  explicit PauseScene(Application& app);

  void Update(engine::time::TimeDelta dt) override;
  void Draw(engine::render::Renderer2D& renderer) override;

 private:
  void BuildUi();
  void LayoutUi(engine::render::Renderer2D& renderer);
  void RefreshVolumeLabels();
  void ToggleOptions();
  void UpdateOptionsButtonLabel();

  Application& app_;
  engine::ui::Canvas canvas_;
  std::shared_ptr<engine::ui::TextElement> title_;
  std::shared_ptr<engine::ui::TextElement> hint_;
  std::shared_ptr<engine::ui::TextElement> music_volume_label_;
  std::shared_ptr<engine::ui::TextElement> sfx_volume_label_;
  std::shared_ptr<ui::Button> resume_button_;
  std::shared_ptr<ui::Button> options_button_;
  std::shared_ptr<ui::Button> quit_button_;
  std::shared_ptr<ui::Button> music_minus_button_;
  std::shared_ptr<ui::Button> music_plus_button_;
  std::shared_ptr<ui::Button> sfx_minus_button_;
  std::shared_ptr<ui::Button> sfx_plus_button_;
  std::vector<std::shared_ptr<ui::Button>> menu_buttons_;
  std::vector<std::shared_ptr<ui::Button>> option_buttons_;
  bool pause_toggle_pressed_{false};
  bool options_open_{true};
};

}  // namespace client

#endif  // CLIENT_SCENE_PAUSE_SCENE_H_
