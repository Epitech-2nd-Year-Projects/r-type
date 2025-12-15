#ifndef CLIENT_SCENE_LOBBY_SCENE_H_
#define CLIENT_SCENE_LOBBY_SCENE_H_

#include <memory>
#include <optional>
#include <vector>

#include "scene.h"
#include "../ui/button.h"
#include "../ui/text_input.h"
#include "../ui/ui_element.h"

namespace client {

class Application;

class LobbyScene : public Scene {
 public:
  explicit LobbyScene(Application& app);

  void Update(engine::time::TimeDelta dt) override;
  void Draw(engine::render::Renderer2D& renderer) override;

 private:
  void LayoutUi(engine::render::Renderer2D& renderer);
  void RefreshRoomButtons();
  void OpenCreateModal();
  void CloseCreateModal();
  void BuildModal();
  void ApplyModalLayout();

  Application& app_;
  std::vector<std::shared_ptr<ui::UIElement>> ui_elements_;
  std::vector<std::shared_ptr<ui::Button>> room_buttons_;
  std::vector<std::shared_ptr<ui::UIElement>> modal_elements_;

  std::shared_ptr<ui::TextInput> host_input_;
  std::shared_ptr<ui::TextInput> port_input_;
  std::shared_ptr<ui::TextInput> name_input_;
  std::shared_ptr<ui::Button> refresh_button_;
  std::shared_ptr<ui::Button> back_button_;
  std::shared_ptr<ui::Button> create_button_;

  std::shared_ptr<ui::TextInput> modal_room_code_input_;
  std::shared_ptr<ui::TextInput> modal_max_players_input_;
  std::shared_ptr<ui::Button> modal_privacy_button_;
  std::shared_ptr<ui::Button> modal_create_button_;
  std::shared_ptr<ui::Button> modal_cancel_button_;
  bool show_modal_{false};
  bool modal_private_{false};

  std::size_t last_rooms_hash_{0};
};

}  // namespace client

#endif  // CLIENT_SCENE_LOBBY_SCENE_H_
