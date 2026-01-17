#ifndef RIFT_CLIENT_SCENE_FIGHT_SCENE_H_
#define RIFT_CLIENT_SCENE_FIGHT_SCENE_H_

#include <memory>

#include "animation_system.h"
#include "engine/render/renderer2d.h"
#include "engine/time/time_delta.h"
#include "fight_camera.h"
#include "fight_hud.h"
#include "rift_context.h"
#include "scene/scene.h"
#include "world_builder.h"

namespace engine::render {
class Renderer3D;
}

namespace rift::client {

class FightScene : public Scene {
 public:
  explicit FightScene(RiftContext& context);

  void Update(engine::time::TimeDelta dt) override;

  void Draw(engine::render::Renderer2D& renderer) override;

  void DrawBackground(engine::render::Renderer2D& renderer) override;

 private:
  void EnsureModelsLoaded();
  void UpdateCamera();

  void Draw3DWorld();
  void DrawArena3D(engine::render::Renderer3D& renderer);
  void DrawFighters3D(engine::render::Renderer3D& renderer);

  void DrawWaitingMessage(engine::render::Renderer2D& renderer);
  void DrawMatchResult(engine::render::Renderer2D& renderer);
  void DrawConnectionStatus(engine::render::Renderer2D& renderer);

  RiftContext& context_;
  FightHud hud_;
  FightCamera camera_;
  std::unique_ptr<AnimationSystem> animation_system_;
  WorldBuilder world_builder_;
  bool models_loaded_{false};
  bool animations_initialized_{false};
};

}  // namespace rift::client

#endif  // RIFT_CLIENT_SCENE_FIGHT_SCENE_H_
