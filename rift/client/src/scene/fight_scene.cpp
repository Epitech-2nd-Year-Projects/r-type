#include "scene/fight_scene.h"

#include <algorithm>

#include "arena_3d_constants.h"
#include "ecs/components.h"
#include "engine/ecs/registry.h"
#include "engine/math/rect.h"
#include "engine/render/color.h"
#include "engine/render/light.h"
#include "engine/render/renderer3d.h"
#include "rift/arena_constants.h"

namespace rift::client {

namespace {

const char* kFighterModelPath =
    "rift/client/assets/animations/Rig_Medium_CombatMelee.glb";

const char* kFighterTexturePath =
    "rift/client/assets/characters/mannequin_texture.png";

}  // namespace

FightScene::FightScene(RiftContext& context)
    : context_(context),
      animation_system_(
          std::make_unique<AnimationSystem>(context.Renderer3D())),
      world_builder_(context.Renderer3D()) {}

void FightScene::Update(engine::time::TimeDelta dt) {
  hud_.Update(context_.World(), context_.LocalPlayerId());
  hud_.SetRoundTimer(context_.RoundTimerMs());
  UpdateCamera();

  if (animations_initialized_) {
    animation_system_->Update(context_.World(), dt, context_.GetInputState(),
                              context_.LocalPlayerId());
  }
}

void FightScene::UpdateCamera() {
  const auto& world = context_.World();
  const auto& positions = world.GetComponents<ecs::PositionComponent>();
  const auto& fighters = world.GetComponents<ecs::FighterStateComponent>();

  float p1_x = Arena3DConstants::kPlayer1SpawnX3D;
  float p2_x = Arena3DConstants::kPlayer2SpawnX3D;

  for (std::size_t i = 0; i < fighters.size(); ++i) {
    if (!fighters[i].has_value()) continue;

    float x_2d = fighters[i]->slot == 0 ? ArenaConstants::kPlayer1SpawnX
                                        : ArenaConstants::kPlayer2SpawnX;

    if (i < positions.size() && positions[i].has_value()) {
      x_2d = positions[i]->render_position.x;
    }

    const float x_3d = Arena3DConstants::To3DX(x_2d);
    if (fighters[i]->slot == 0) {
      p1_x = x_3d;
    } else {
      p2_x = x_3d;
    }
  }

  camera_.UpdateTarget(p1_x, p2_x);
}

void FightScene::EnsureModelsLoaded() {
  if (models_loaded_) return;

  auto& renderer3d = context_.Renderer3D();
  auto& world = context_.World();
  auto& fighters = world.GetComponents<ecs::FighterStateComponent>();
  auto& render3d = world.GetComponents<ecs::Fighter3DRenderComponent>();

  for (std::size_t i = 0; i < fighters.size(); ++i) {
    if (!fighters[i].has_value()) continue;

    if (!render3d[i].has_value()) {
      render3d[i] = ecs::Fighter3DRenderComponent{};
    }

    if (!render3d[i]->model) {
      render3d[i]->model = renderer3d.LoadModelFromFile(kFighterModelPath);
      render3d[i]->scale = Arena3DConstants::kFighterModelScale;

      if (render3d[i]->model) {
        renderer3d.SetModelTexture(*render3d[i]->model, kFighterTexturePath);
      }
    }
  }

  if (!world_builder_.IsInitialized()) {
    world_builder_.Initialize(Arena3DConstants::kArenaWidth3D,
                              Arena3DConstants::kArenaDepth3D);
  }

  models_loaded_ = true;

  if (!animations_initialized_) {
    animations_initialized_ = animation_system_->Initialize();
  }
}

void FightScene::Draw(engine::render::Renderer2D& renderer) {
  const auto state = context_.State();

  switch (state) {
    case RiftClientState::kConnecting:
      DrawConnectionStatus(renderer);
      break;

    case RiftClientState::kWaitingRoom:
      DrawWaitingMessage(renderer);
      break;

    case RiftClientState::kPlaying:
      EnsureModelsLoaded();
      Draw3DWorld();
      hud_.Draw(renderer, context_.RenderSize());
      break;

    case RiftClientState::kMatchOver:
      EnsureModelsLoaded();
      Draw3DWorld();
      hud_.Draw(renderer, context_.RenderSize());
      DrawMatchResult(renderer);
      break;
  }
}

void FightScene::DrawBackground(engine::render::Renderer2D& renderer) {
  const auto size = context_.RenderSize();
  renderer.DrawRect(
      {0.0f, 0.0f, static_cast<float>(size.x), static_cast<float>(size.y)},
      engine::render::Color::FromBytes(20, 20, 30));
}

void FightScene::Draw3DWorld() {
  auto& renderer3d = context_.Renderer3D();
  auto& renderer2d = context_.Renderer();
  const auto size = context_.RenderSize();

  world_builder_.DrawSky(renderer2d, size.x, size.y);

  if (animations_initialized_) {
    animation_system_->ApplyAnimations(context_.World());
  }

  renderer3d.Begin3D(camera_.GetCamera());

  engine::render::LightingConfig lighting;
  lighting.ambient.color = engine::render::Color::White();
  lighting.ambient.intensity = 0.4f;
  lighting.directional.direction = {-0.3f, -1.0f, -0.5f};
  lighting.directional.color = engine::render::Color::FromBytes(255, 250, 240);
  lighting.directional.intensity = 0.8f;
  lighting.directional.enabled = true;
  renderer3d.SetLighting(lighting);

  DrawArena3D(renderer3d);
  DrawFighters3D(renderer3d);

  renderer3d.End3D();
}

void FightScene::DrawArena3D(engine::render::Renderer3D& renderer) {
  world_builder_.Draw(renderer);

  const float half_width = Arena3DConstants::kArenaWidth3D / 2.0f;
  const float half_depth = Arena3DConstants::kArenaDepth3D / 2.0f;

  renderer.DrawLine3D({-half_width, 0.01f, -half_depth},
                      {-half_width, 0.01f, half_depth},
                      engine::render::Color::FromBytes(100, 80, 60));
  renderer.DrawLine3D({half_width, 0.01f, -half_depth},
                      {half_width, 0.01f, half_depth},
                      engine::render::Color::FromBytes(100, 80, 60));
}

void FightScene::DrawFighters3D(engine::render::Renderer3D& renderer) {
  const auto& world = context_.World();
  const auto& positions = world.GetComponents<ecs::PositionComponent>();
  const auto& fighters = world.GetComponents<ecs::FighterStateComponent>();
  const auto& render3d = world.GetComponents<ecs::Fighter3DRenderComponent>();

  for (std::size_t i = 0; i < fighters.size(); ++i) {
    if (!fighters[i].has_value()) continue;

    float x_2d = fighters[i]->slot == 0 ? ArenaConstants::kPlayer1SpawnX
                                        : ArenaConstants::kPlayer2SpawnX;

    if (i < positions.size() && positions[i].has_value()) {
      x_2d = positions[i]->render_position.x;
    }

    const float x_3d = Arena3DConstants::To3DX(x_2d);

    const float rotation_y = fighters[i]->facing_right ? 90.0f : -90.0f;

    const bool is_local =
        context_.LocalPlayerId().has_value() &&
        fighters[i]->player_id == context_.LocalPlayerId().value();

    engine::render::Color tint;
    if (fighters[i]->slot == 0) {
      tint = engine::render::Color::FromBytes(180, 200, 255);
    } else {
      tint = engine::render::Color::FromBytes(255, 180, 180);
    }
    if (is_local) {
      tint = engine::render::Color::FromBytes(
          std::min(255, static_cast<int>(tint.r * 255) + 40),
          std::min(255, static_cast<int>(tint.g * 255) + 40),
          std::min(255, static_cast<int>(tint.b * 255) + 40));
    }

    if (i < render3d.size() && render3d[i].has_value() && render3d[i]->model) {
      engine::render::ModelDrawParams params;
      params.position = {x_3d, Arena3DConstants::kFighterBaseY, 0.0f};
      params.rotation_axis = {0.0f, 1.0f, 0.0f};
      params.rotation_angle = rotation_y;
      params.scale = {render3d[i]->scale, render3d[i]->scale,
                      render3d[i]->scale};
      params.tint = tint;

      renderer.DrawModel(*render3d[i]->model, params);
    } else {
      engine::render::Color color =
          fighters[i]->slot == 0
              ? engine::render::Color::FromBytes(50, 100, 255)
              : engine::render::Color::FromBytes(255, 80, 80);

      renderer.DrawCube({x_3d, 1.0f, 0.0f}, {1.0f, 2.0f, 0.5f}, color);
    }
  }
}

void FightScene::DrawWaitingMessage(engine::render::Renderer2D& renderer) {
  const auto size = context_.RenderSize();
  const float center_x = static_cast<float>(size.x) / 2.0f;
  const float center_y = static_cast<float>(size.y) / 2.0f;

  const float box_width = 400.0f;
  const float box_height = 100.0f;

  renderer.DrawRect({center_x - box_width / 2.0f, center_y - box_height / 2.0f,
                     box_width, box_height},
                    engine::render::Color::FromBytes(20, 20, 40, 220));

  renderer.DrawText("Waiting for opponent...",
                    {center_x - 100.0f, center_y - 10.0f}, 20.0f,
                    engine::render::Color::White());
}

void FightScene::DrawMatchResult(engine::render::Renderer2D& renderer) {
  const auto size = context_.RenderSize();
  const float center_x = static_cast<float>(size.x) / 2.0f;
  const float center_y = static_cast<float>(size.y) / 2.0f;

  const float box_width = 500.0f;
  const float box_height = 150.0f;

  renderer.DrawRect({center_x - box_width / 2.0f, center_y - box_height / 2.0f,
                     box_width, box_height},
                    engine::render::Color::FromBytes(20, 20, 20, 240));

  renderer.DrawText("MATCH OVER", {center_x - 60.0f, center_y - 10.0f}, 24.0f,
                    engine::render::Color::FromBytes(255, 215, 0));
}

void FightScene::DrawConnectionStatus(engine::render::Renderer2D& renderer) {
  const auto size = context_.RenderSize();
  const float center_x = static_cast<float>(size.x) / 2.0f;
  const float center_y = static_cast<float>(size.y) / 2.0f;

  const float box_width = 300.0f;
  const float box_height = 80.0f;

  renderer.DrawRect({center_x - box_width / 2.0f, center_y - box_height / 2.0f,
                     box_width, box_height},
                    engine::render::Color::FromBytes(20, 40, 60, 220));

  renderer.DrawText("Connecting...", {center_x - 50.0f, center_y - 10.0f},
                    20.0f, engine::render::Color::White());
}

}  // namespace rift::client
