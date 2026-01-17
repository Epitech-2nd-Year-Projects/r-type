#include "scene/fight_scene.h"

#include "ecs/components.h"
#include "engine/ecs/registry.h"
#include "engine/math/rect.h"
#include "engine/render/color.h"

namespace rift::client {

namespace {

constexpr float kArenaFloorY = 500.0f;
constexpr float kFighterWidth = 60.0f;
constexpr float kFighterHeight = 120.0f;

}  // namespace

FightScene::FightScene(RiftContext& context) : context_(context) {}

void FightScene::Update(engine::time::TimeDelta dt) {
  hud_.Update(context_.World(), context_.LocalPlayerId());
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
      DrawFighters(renderer);
      hud_.Draw(renderer, context_.RenderSize());
      break;

    case RiftClientState::kMatchOver:
      DrawFighters(renderer);
      hud_.Draw(renderer, context_.RenderSize());
      DrawMatchResult(renderer);
      break;
  }
}

void FightScene::DrawBackground(engine::render::Renderer2D& renderer) {
  const auto size = context_.RenderSize();

  renderer.DrawRect(
      {0.0f, 0.0f, static_cast<float>(size.x), static_cast<float>(size.y)},
      engine::render::Color::FromBytes(30, 30, 50));

  renderer.DrawRect({0.0f, kArenaFloorY, static_cast<float>(size.x),
                     static_cast<float>(size.y) - kArenaFloorY},
                    engine::render::Color::FromBytes(60, 40, 30));

  renderer.DrawLine({0.0f, kArenaFloorY},
                    {static_cast<float>(size.x), kArenaFloorY}, 2.0f,
                    engine::render::Color::FromBytes(100, 80, 60));
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

void FightScene::DrawFighters(engine::render::Renderer2D& renderer) {
  const auto& world = context_.World();
  const auto& positions = world.GetComponents<ecs::PositionComponent>();
  const auto& fighters = world.GetComponents<ecs::FighterStateComponent>();
  const auto& renders = world.GetComponents<ecs::FighterRenderComponent>();

  for (std::size_t i = 0; i < fighters.size(); ++i) {
    if (!fighters[i].has_value()) continue;

    float x = 0.0f;
    float y = kArenaFloorY - kFighterHeight;

    if (i < positions.size() && positions[i].has_value()) {
      x = positions[i]->render_position.x;
      y = positions[i]->render_position.y;
    } else {
      x = fighters[i]->slot == 0 ? 340.0f : 880.0f;
    }

    engine::render::Color color =
        fighters[i]->slot == 0
            ? engine::render::Color::FromBytes(50, 100, 255)
            : engine::render::Color::FromBytes(255, 80, 80);

    float width = kFighterWidth;
    float height = kFighterHeight;

    if (i < renders.size() && renders[i].has_value()) {
      width = renders[i]->width;
      height = renders[i]->height;
      color = renders[i]->color;
    }

    const bool is_local =
        context_.LocalPlayerId().has_value() &&
        fighters[i]->player_id == context_.LocalPlayerId().value();

    if (is_local) {
      renderer.DrawRect({x - 5.0f, y - 5.0f, width + 10.0f, height + 10.0f},
                        engine::render::Color::FromBytes(255, 255, 0));
    }

    renderer.DrawRect({x, y, width, height}, color);
  }
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
