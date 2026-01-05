#ifndef CLIENT_SCENE_PROFILE_SCENE_H_
#define CLIENT_SCENE_PROFILE_SCENE_H_

#include <memory>
#include <string>

#include "scene.h"
#include "engine/ui/button.h"
#include "engine/ui/canvas.h"
#include "engine/ui/text.h"
#include "engine/ui/text_input.h"
#include "engine/ui/widget.h"

namespace client {

class ClientContext;

/**
 * @brief Scene for editing player profile (nickname and viewing stats)
 */
class ProfileScene : public Scene {
 public:
  explicit ProfileScene(ClientContext& context);

  void Update(engine::time::TimeDelta dt) override;
  void Draw(engine::render::Renderer2D& renderer) override;
  bool IsInputCaptured() const override { return text_input_focused_; }

 private:
  void LayoutUi(engine::render::Renderer2D& renderer);
  void SaveAndClose();
  void FormatPlaytime(std::uint64_t seconds, std::string& out) const;

  ClientContext& context_;
  engine::ui::Canvas canvas_;

  std::shared_ptr<engine::ui::TextElement> title_;
  std::shared_ptr<engine::ui::TextElement> nickname_label_;
  std::shared_ptr<engine::ui::TextInput> nickname_input_;
  std::shared_ptr<engine::ui::TextElement> stats_header_;
  std::shared_ptr<engine::ui::TextElement> playtime_text_;
  std::shared_ptr<engine::ui::TextElement> deaths_text_;
  std::shared_ptr<engine::ui::TextElement> highest_score_text_;
  std::shared_ptr<engine::ui::TextElement> games_played_text_;
  std::shared_ptr<engine::ui::Button> save_button_;
  std::shared_ptr<engine::ui::Button> back_button_;

  std::vector<std::shared_ptr<engine::ui::Widget>> ui_elements_;
  bool text_input_focused_{false};
};

}  // namespace client

#endif  // CLIENT_SCENE_PROFILE_SCENE_H_
