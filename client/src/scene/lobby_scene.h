#ifndef CLIENT_SCENE_LOBBY_SCENE_H_
#define CLIENT_SCENE_LOBBY_SCENE_H_

#include <chrono>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "../ui/button.h"
#include "../ui/text_input.h"
#include "../ui/ui_element.h"
#include "protocol/lobby.h"
#include "scene.h"

namespace client {

class Application;

class LobbyScene : public Scene {
 public:
  explicit LobbyScene(Application& app);

  void Update(engine::time::TimeDelta dt) override;
  void Draw(engine::render::Renderer2D& renderer) override;
  bool IsInputCaptured() const override;

 private:
  void LayoutUi(engine::render::Renderer2D& renderer);
  void RefreshRoomButtons();
  void OpenCreateModal();
  void CloseModal();
  void BuildModal();
  void ApplyModalLayout();
  void OpenJoinModal(const protocol::RoomSummary& room);
  void JoinRoom(const protocol::RoomSummary& room, const std::string& password);

  enum class ModalMode { kCreate, kJoinPrivate };

  Application& app_;
  std::vector<std::shared_ptr<ui::UIElement>> ui_elements_;
  std::vector<std::shared_ptr<ui::Button>> room_buttons_;
  std::vector<std::shared_ptr<ui::UIElement>> modal_elements_;
  std::vector<std::shared_ptr<ui::UIElement>> create_modal_elements_;
  std::vector<std::shared_ptr<ui::UIElement>> join_modal_elements_;

  std::shared_ptr<ui::TextInput> host_input_;
  std::shared_ptr<ui::TextInput> port_input_;
  std::shared_ptr<ui::TextInput> name_input_;
  std::shared_ptr<ui::Button> refresh_button_;
  std::shared_ptr<ui::Button> create_button_;

  std::shared_ptr<ui::TextInput> modal_room_name_input_;
  std::shared_ptr<ui::TextInput> modal_max_players_input_;
  std::shared_ptr<ui::Button> modal_privacy_button_;
  std::shared_ptr<ui::TextInput> modal_password_input_;
  std::shared_ptr<ui::Button> modal_primary_button_;
  std::shared_ptr<ui::Button> modal_cancel_button_;
  bool show_modal_{false};
  bool modal_private_{false};
  ModalMode modal_mode_{ModalMode::kCreate};
  std::string pending_join_room_code_;
  std::string pending_join_room_name_;

  std::string lobby_host_;
  std::uint16_t lobby_port_{0};

  std::string banner_text_;
  std::chrono::steady_clock::time_point banner_expiry_{};

  std::size_t last_rooms_hash_{0};
};

}  // namespace client

#endif  // CLIENT_SCENE_LOBBY_SCENE_H_
