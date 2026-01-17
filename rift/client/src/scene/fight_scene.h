#ifndef RIFT_CLIENT_SCENE_FIGHT_SCENE_H_
#define RIFT_CLIENT_SCENE_FIGHT_SCENE_H_

#include "engine/render/renderer2d.h"
#include "engine/time/time_delta.h"
#include "fight_hud.h"
#include "rift_context.h"
#include "scene/scene.h"

namespace rift::client {

class FightScene : public Scene {
 public:
  explicit FightScene(RiftContext& context);

  void Update(engine::time::TimeDelta dt) override;

  void Draw(engine::render::Renderer2D& renderer) override;

  void DrawBackground(engine::render::Renderer2D& renderer) override;

 private:
  void DrawWaitingMessage(engine::render::Renderer2D& renderer);
  void DrawFighters(engine::render::Renderer2D& renderer);
  void DrawMatchResult(engine::render::Renderer2D& renderer);
  void DrawConnectionStatus(engine::render::Renderer2D& renderer);

  RiftContext& context_;
  FightHud hud_;
};

}  // namespace rift::client

#endif  // RIFT_CLIENT_SCENE_FIGHT_SCENE_H_
