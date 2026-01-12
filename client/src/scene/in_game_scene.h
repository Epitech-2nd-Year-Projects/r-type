#ifndef CLIENT_SCENE_IN_GAME_SCENE_H_
#define CLIENT_SCENE_IN_GAME_SCENE_H_

#include "hud_overlay.h"
#include "lobby_chat_view.h"
#include "scene.h"

namespace client {

class ClientContext;

class InGameScene : public Scene {
 public:
  explicit InGameScene(ClientContext& context);

  void Update(engine::time::TimeDelta dt) override;
  void Draw(engine::render::Renderer2D& renderer) override;
  bool IsInputCaptured() const override;

 private:
  void LayoutChat();

  ClientContext& context_;
  HudOverlay hud_;
  LobbyChatView chat_view_;
  bool is_ready_{false};
  bool toggle_pressed_{false};
  bool pause_pressed_{false};
};

}  // namespace client

#endif  // CLIENT_SCENE_IN_GAME_SCENE_H_

