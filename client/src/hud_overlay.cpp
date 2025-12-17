/**
 * @file hud_overlay.cpp
 * @brief HUD overlay drawing implementation
 */

#include "hud_overlay.h"

#include <algorithm>
#include <iomanip>
#include <sstream>
#include <string>
#include <utility>

#include "ecs/components.h"
#include "engine/ecs/registry.h"
#include "engine/math/rect.h"
#include "engine/math/vector2.h"
#include "engine/render/color.h"
#include "engine/render/renderer2d.h"

namespace client {
namespace {

constexpr std::uint16_t kPlayerTypeCode = 1u;
constexpr float kPanelMargin = 16.0f;
constexpr float kPanelPadding = 12.0f;
constexpr float kLineSpacing = 6.0f;
constexpr float kHeaderFontSize = 20.0f;
constexpr float kBodyFontSize = 18.0f;
constexpr float kIndicatorRadius = 7.0f;

const engine::render::Color kPanelBackground =
    engine::render::Color::FromBytes(10, 12, 16, 205);
const engine::render::Color kHeaderColor =
    engine::render::Color::FromBytes(230, 235, 245);
const engine::render::Color kBodyColor =
    engine::render::Color::FromBytes(214, 222, 230);
const engine::render::Color kMutedColor =
    engine::render::Color::FromBytes(150, 160, 170);
const engine::render::Color kLocalColor =
    engine::render::Color::FromBytes(120, 190, 255);
const engine::render::Color kConnectedColor =
    engine::render::Color::FromBytes(84, 199, 136);
const engine::render::Color kWarningColor =
    engine::render::Color::FromBytes(236, 195, 86);
const engine::render::Color kProblemColor =
    engine::render::Color::FromBytes(214, 89, 82);
const engine::render::Color kOfflineColor =
    engine::render::Color::FromBytes(120, 130, 140);

struct TextLine {
  std::string text;
  float font_size;
  engine::render::Color color;
};

engine::render::Color ConnectionColor(std::optional<float> latency_ms,
                                      bool connected) {
  if (!connected) {
    return kOfflineColor;
  }
  if (!latency_ms.has_value()) {
    return kConnectedColor;
  }
  if (*latency_ms < 80.0f) {
    return kConnectedColor;
  }
  if (*latency_ms < 150.0f) {
    return kWarningColor;
  }
  return kProblemColor;
}

float MaxTextWidth(const std::vector<TextLine>& lines,
                   engine::render::Renderer2D& renderer) {
  float width = 0.0f;
  for (const auto& line : lines) {
    width = std::max(width, renderer.MeasureText(line.text, line.font_size).x);
  }
  return width;
}

float PanelHeight(const std::vector<TextLine>& lines) {
  float height = kPanelPadding * 2.0f;
  for (std::size_t i = 0; i < lines.size(); ++i) {
    height += lines[i].font_size;
    if (i + 1 < lines.size()) {
      height += kLineSpacing;
    }
  }
  return height;
}

std::string FormatNumber(std::optional<std::uint32_t> value) {
  if (!value.has_value()) {
    return "--";
  }
  return std::to_string(*value);
}

std::string FormatHealth(const HudPlayerRow& row) {
  if (!row.hp.has_value() || !row.max_hp.has_value()) {
    return "HP --/--";
  }
  std::ostringstream stream;
  stream << "HP " << row.hp.value() << "/" << row.max_hp.value();
  return stream.str();
}

std::string FormatPlayerLine(const HudPlayerRow& row) {
  std::ostringstream stream;
  if (row.is_local) {
    stream << "> ";
  } else {
    stream << "  ";
  }
  stream << "P" << row.player_id << "  " << FormatHealth(row) << "  Lives "
         << FormatNumber(row.lives) << "  Score " << FormatNumber(row.score);
  if (!row.alive) {
    stream << " (down)";
  } else if (!row.is_ready) {
    stream << " (not ready)";
  } else {
    stream << " (READY)";
  }
  return stream.str();
}

std::string FormatPing(std::optional<float> latency_ms, bool connected) {
  if (!connected) {
    return "Ping: --";
  }
  if (!latency_ms.has_value()) {
    return "Ping: ...";
  }
  std::ostringstream stream;
  stream << "Ping: " << std::fixed << std::setprecision(1) << *latency_ms
         << " ms";
  return stream.str();
}

}  // namespace

void HudOverlay::UpdatePlayers(const engine::ecs::Registry& registry,
                               std::optional<std::uint32_t> local_player_id) {
  players_.clear();

  const auto& net = registry.GetComponents<ecs::NetworkedEntityComponent>();
  const auto& health = registry.GetComponents<ecs::HealthComponent>();
  const auto& player_state =
      registry.GetComponents<ecs::PlayerStateComponent>();
  players_.reserve(net.size());

  for (std::size_t i = 0; i < net.size(); ++i) {
    if (!net[i].has_value()) {
      continue;
    }
    const auto& comp = net[i].value();
    if (comp.type_code != kPlayerTypeCode) {
      continue;
    }
    HudPlayerRow row;
    row.player_id = comp.network_id;
    row.is_ready = (net[i]->flags & 2u) != 0;
    if (i < player_state.size() && player_state[i].has_value()) {
      if (player_state[i]->player_id != 0u) {
        row.player_id = player_state[i]->player_id;
      }
      row.score = player_state[i]->score;
      row.lives = static_cast<std::uint32_t>(player_state[i]->lives);
    }
    row.is_local =
        local_player_id.has_value() && row.player_id == *local_player_id;
    const bool has_health = i < health.size() && health[i].has_value();
    if (has_health) {
      row.hp = static_cast<std::uint32_t>(health[i]->current);
      row.max_hp = static_cast<std::uint32_t>(health[i]->max);
      row.alive = health[i]->current > 0;
    }
    if (!has_health && row.lives.has_value()) {
      row.alive = *row.lives > 0;
    }
    players_.push_back(row);
  }

  if (players_.empty() && local_player_id.has_value()) {
    HudPlayerRow placeholder;
    placeholder.player_id = *local_player_id;
    placeholder.is_local = true;
    players_.push_back(placeholder);
  }

  std::sort(players_.begin(), players_.end(),
            [](const auto& lhs, const auto& rhs) {
              return lhs.player_id < rhs.player_id;
            });
}

void HudOverlay::UpdateWave(std::optional<std::uint32_t> wave) {
  current_wave_ = wave;
}

void HudOverlay::UpdateNetwork(std::optional<float> latency_ms, bool connected,
                               std::string status_text) {
  latency_ms_ = latency_ms;
  connected_ = connected;
  status_text_ = std::move(status_text);
  if (status_text_.empty()) {
    status_text_ = connected_ ? "Connected" : "Disconnected";
  }
}

void HudOverlay::Draw(engine::render::Renderer2D& renderer,
                      const engine::math::Vector2i& window_size) const {
  const engine::math::Vector2f player_origin{kPanelMargin, kPanelMargin};

  std::vector<TextLine> player_lines;
  player_lines.push_back(
      {"Wave " + FormatNumber(current_wave_), kHeaderFontSize, kHeaderColor});

  if (players_.empty()) {
    player_lines.push_back({"Waiting for players", kBodyFontSize, kMutedColor});
  } else {
    for (const auto& row : players_) {
      const engine::render::Color line_color =
          row.alive ? (row.is_local ? kLocalColor : kBodyColor) : kMutedColor;
      player_lines.push_back(
          {FormatPlayerLine(row), kBodyFontSize, line_color});
    }
  }

  const float player_width = MaxTextWidth(player_lines, renderer);
  const float player_height = PanelHeight(player_lines);
  const engine::math::RectF player_background{
      player_origin.x, player_origin.y, player_width + kPanelPadding * 2.0f,
      player_height};
  renderer.DrawRect(player_background, kPanelBackground);

  float y = player_origin.y + kPanelPadding;
  const float text_x = player_origin.x + kPanelPadding;
  for (const auto& line : player_lines) {
    renderer.DrawText(line.text, {text_x, y}, line.font_size, line.color);
    y += line.font_size + kLineSpacing;
  }

  const std::string ping_line = FormatPing(latency_ms_, connected_);
  const engine::render::Color indicator_color =
      ConnectionColor(latency_ms_, connected_);
  const std::vector<TextLine> network_lines = {
      {status_text_, kBodyFontSize, indicator_color},
      {ping_line, kBodyFontSize, kBodyColor},
  };

  const float text_indent = kIndicatorRadius * 2.0f + 8.0f;
  const float network_text_width = MaxTextWidth(network_lines, renderer);
  const float network_width =
      text_indent + network_text_width + kPanelPadding * 2.0f;
  const float network_height = PanelHeight(network_lines);
  const float network_origin_x =
      std::max(kPanelMargin, static_cast<float>(window_size.x) - network_width -
                                 kPanelMargin);
  const engine::math::Vector2f network_origin{network_origin_x, kPanelMargin};

  const engine::math::RectF network_background{
      network_origin.x, network_origin.y, network_width, network_height};
  renderer.DrawRect(network_background, kPanelBackground);

  const float indicator_center_x =
      network_origin.x + kPanelPadding + kIndicatorRadius;
  const float indicator_center_y =
      network_origin.y + kPanelPadding + (kBodyFontSize / 2.0f);
  renderer.DrawCircle({indicator_center_x, indicator_center_y},
                      kIndicatorRadius, indicator_color);

  float network_y = network_origin.y + kPanelPadding;
  const float network_text_x = network_origin.x + kPanelPadding + text_indent;
  for (const auto& line : network_lines) {
    renderer.DrawText(line.text, {network_text_x, network_y}, line.font_size,
                      line.color);
    network_y += line.font_size + kLineSpacing;
  }
}

}  // namespace client
