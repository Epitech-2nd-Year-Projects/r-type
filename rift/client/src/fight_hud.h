#ifndef RIFT_CLIENT_FIGHT_HUD_H_
#define RIFT_CLIENT_FIGHT_HUD_H_

#include <cstdint>
#include <optional>

#include "engine/math/vector2.h"
#include "engine/render/renderer2d.h"

namespace engine::ecs {
class Registry;
}

namespace rift::client {

class FightHud {
 public:
  FightHud() = default;

  void Update(const engine::ecs::Registry& world,
              std::optional<std::uint32_t> local_player_id);

  void Draw(engine::render::Renderer2D& renderer,
            engine::math::Vector2i render_size);

 private:
  void DrawHealthBar(engine::render::Renderer2D& renderer, float x, float y,
                     float width, float height, float percent, bool flip);

  void DrawStaminaBar(engine::render::Renderer2D& renderer, float x, float y,
                      float width, float height, float percent, bool flip);

  void DrawRoundIndicators(engine::render::Renderer2D& renderer, float x,
                           float y, int rounds_won, bool flip);

  void DrawTimer(engine::render::Renderer2D& renderer, float x, float y,
                 std::uint32_t timer_ms);

  struct FighterHudData {
    std::uint32_t player_id{0};
    float health_percent{1.0f};
    float stamina_percent{1.0f};
    std::uint8_t rounds_won{0};
    std::uint8_t slot{0};
  };

  std::optional<FighterHudData> player1_;
  std::optional<FighterHudData> player2_;
  std::uint32_t round_timer_ms_{0};
};

}  // namespace rift::client

#endif  // RIFT_CLIENT_FIGHT_HUD_H_
