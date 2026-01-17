#include "fight_hud.h"

#include "ecs/components.h"
#include "engine/ecs/registry.h"
#include "engine/math/rect.h"
#include "engine/render/color.h"

namespace rift::client {

namespace {

constexpr float kHealthBarWidth = 400.0f;
constexpr float kHealthBarHeight = 30.0f;
constexpr float kStaminaBarWidth = 350.0f;
constexpr float kStaminaBarHeight = 15.0f;
constexpr float kHudMargin = 50.0f;
constexpr float kHudTopMargin = 30.0f;
constexpr float kBarSpacing = 5.0f;
constexpr float kRoundIndicatorSize = 20.0f;
constexpr float kRoundIndicatorSpacing = 10.0f;

}  // namespace

void FightHud::Update(const engine::ecs::Registry& world,
                      std::optional<std::uint32_t> local_player_id) {
  player1_.reset();
  player2_.reset();

  const auto& fighters = world.GetComponents<ecs::FighterStateComponent>();
  const auto& health = world.GetComponents<ecs::HealthBarComponent>();
  const auto& stamina = world.GetComponents<ecs::StaminaBarComponent>();

  for (std::size_t i = 0; i < fighters.size(); ++i) {
    if (!fighters[i].has_value()) continue;

    FighterHudData data;
    data.player_id = fighters[i]->player_id;
    data.slot = fighters[i]->slot;
    data.rounds_won = fighters[i]->rounds_won;

    if (i < health.size() && health[i].has_value()) {
      data.health_percent = health[i]->Percent();
    }

    if (i < stamina.size() && stamina[i].has_value()) {
      data.stamina_percent = stamina[i]->Percent();
    }

    if (data.slot == 0) {
      player1_ = data;
    } else if (data.slot == 1) {
      player2_ = data;
    }
  }
}

void FightHud::Draw(engine::render::Renderer2D& renderer,
                    engine::math::Vector2i render_size) {
  const float screen_width = static_cast<float>(render_size.x);

  const int p1_lives_lost = player2_.has_value() ? player2_->rounds_won : 0;
  const int p2_lives_lost = player1_.has_value() ? player1_->rounds_won : 0;

  if (player1_.has_value()) {
    DrawHealthBar(renderer, kHudMargin, kHudTopMargin, kHealthBarWidth,
                  kHealthBarHeight, player1_->health_percent, false);

    DrawStaminaBar(renderer, kHudMargin,
                   kHudTopMargin + kHealthBarHeight + kBarSpacing,
                   kStaminaBarWidth, kStaminaBarHeight,
                   player1_->stamina_percent, false);

    DrawLifeIndicators(renderer, kHudMargin,
                       kHudTopMargin + kHealthBarHeight + kStaminaBarHeight +
                           kBarSpacing * 2,
                       p1_lives_lost, false);
  }

  if (player2_.has_value()) {
    DrawHealthBar(renderer, screen_width - kHudMargin - kHealthBarWidth,
                  kHudTopMargin, kHealthBarWidth, kHealthBarHeight,
                  player2_->health_percent, true);

    DrawStaminaBar(
        renderer, screen_width - kHudMargin - kStaminaBarWidth,
        kHudTopMargin + kHealthBarHeight + kBarSpacing, kStaminaBarWidth,
        kStaminaBarHeight, player2_->stamina_percent, true);

    DrawLifeIndicators(
        renderer,
        screen_width - kHudMargin - kRoundIndicatorSize * 2 -
            kRoundIndicatorSpacing,
        kHudTopMargin + kHealthBarHeight + kStaminaBarHeight + kBarSpacing * 2,
        p2_lives_lost, true);
  }

  DrawTimer(renderer, screen_width / 2.0f, kHudTopMargin, round_timer_ms_);
}

void FightHud::DrawHealthBar(engine::render::Renderer2D& renderer, float x,
                             float y, float width, float height, float percent,
                             bool flip) {
  renderer.DrawRect({x, y, width, height},
                    engine::render::Color::FromBytes(40, 40, 40));

  const float fill_width = width * percent;
  const float fill_x = flip ? x + width - fill_width : x;

  engine::render::Color health_color =
      percent > 0.3f ? engine::render::Color::FromBytes(0, 200, 0)
                     : engine::render::Color::FromBytes(200, 0, 0);

  renderer.DrawRect({fill_x, y, fill_width, height}, health_color);
}

void FightHud::DrawStaminaBar(engine::render::Renderer2D& renderer, float x,
                              float y, float width, float height, float percent,
                              bool flip) {
  renderer.DrawRect({x, y, width, height},
                    engine::render::Color::FromBytes(30, 30, 30));

  const float fill_width = width * percent;
  const float fill_x = flip ? x + width - fill_width : x;

  renderer.DrawRect({fill_x, y, fill_width, height},
                    engine::render::Color::FromBytes(255, 200, 0));
}

void FightHud::DrawLifeIndicators(engine::render::Renderer2D& renderer, float x,
                                  float y, int lives_lost, bool flip) {
  const int max_lives = 2;
  for (int i = 0; i < max_lives; ++i) {
    const float indicator_x =
        flip ? x - static_cast<float>(i) *
                       (kRoundIndicatorSize + kRoundIndicatorSpacing)
             : x + static_cast<float>(i) *
                       (kRoundIndicatorSize + kRoundIndicatorSpacing);

    engine::render::Color color =
        i < lives_lost ? engine::render::Color::FromBytes(60, 60, 60)
                       : engine::render::Color::FromBytes(200, 0, 0);

    renderer.DrawRect({indicator_x, y, kRoundIndicatorSize, kRoundIndicatorSize},
                      color);
  }
}

void FightHud::DrawTimer(engine::render::Renderer2D& renderer, float x, float y,
                         std::uint32_t timer_ms) {
  const float timer_width = 60.0f;
  const float timer_height = 40.0f;
  const float font_size = 28.0f;

  renderer.DrawRect({x - timer_width / 2.0f, y, timer_width, timer_height},
                    engine::render::Color::FromBytes(20, 20, 20));

  const std::uint32_t seconds = timer_ms / 1000;
  const std::string timer_text = std::to_string(seconds);

  const auto text_size = renderer.MeasureText(timer_text, font_size);
  const float text_x = x - text_size.x / 2.0f;
  const float text_y = y + (timer_height - text_size.y) / 2.0f;

  renderer.DrawText(timer_text, {text_x, text_y}, font_size,
                    engine::render::Color::White());
}

}  // namespace rift::client
