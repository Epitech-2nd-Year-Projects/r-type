#ifndef CLIENT_SCENE_LOBBY_SCENE_H_
#define CLIENT_SCENE_LOBBY_SCENE_H_

#include "lobby_controller.h"
#include "lobby_modal.h"
#include "lobby_room_list_view.h"
#include "scene.h"

namespace protocol {
struct RoomSummary;
}

namespace client {

class ClientContext;

class LobbyScene : public Scene {
 public:
  explicit LobbyScene(ClientContext& context);

  void Update(engine::time::TimeDelta dt) override;
  void Draw(engine::render::Renderer2D& renderer) override;

  /**
   * @brief Draw lobby background
   * @param renderer Renderer instance
   */
  void DrawBackground(engine::render::Renderer2D& renderer) override;

  /**
   * @brief Draw lobby foreground
   * @param renderer Renderer instance
   */
  void DrawForeground(engine::render::Renderer2D& renderer) override;

  bool IsInputCaptured() const override;

 private:
  void LayoutUi();
  void HandleRoomSelected(const protocol::RoomSummary& room);

  ClientContext& context_;
  LobbyController controller_;
  LobbyRoomListView room_list_view_;
  LobbyModal modal_;
};

}  // namespace client

#endif  // CLIENT_SCENE_LOBBY_SCENE_H_
