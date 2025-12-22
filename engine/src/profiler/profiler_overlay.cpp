#include "engine/profiler/profiler_overlay.h"

#include <iomanip>
#include <sstream>

#include "engine/math/rect.h"

namespace engine::profiler {

namespace {
constexpr float kFontSize = 18.0f;
constexpr float kLineSpacing = 6.0f;
constexpr float kPadding = 12.0f;
constexpr float kCornerOffset = 16.0f;
constexpr float kGraphHeight = 40.0f;
const render::Color kBackgroundColor =
    render::Color::FromBytes(12, 14, 18, 210);
const render::Color kTextColor = render::Color::FromBytes(220, 225, 232);
const render::Color kGraphColor = render::Color::FromBytes(0, 200, 100);
const render::Color kGraphBgColor = render::Color::FromBytes(30, 30, 35);
}  // namespace

void ProfilerOverlay::Toggle() { enabled_ = !enabled_; }
void ProfilerOverlay::SetEnabled(bool enabled) { enabled_ = enabled; }
bool ProfilerOverlay::IsEnabled() const { return enabled_; }

void ProfilerOverlay::Draw(render::Renderer2D& renderer,
                           const math::Vector2i& window_size) {
  if (!enabled_) {
    return;
  }

  auto& profiler = Profiler::Get();
  auto metrics = profiler.GetMetrics();
  auto histories = profiler.GetHistories();

  std::vector<std::string> lines;
  std::vector<std::string> graph_names;

  for (const auto& [name, value] : metrics) {
    std::ostringstream ss;
    ss << name << ": ";
    if (std::holds_alternative<float>(value)) {
      ss << std::fixed << std::setprecision(2) << std::get<float>(value);
    } else if (std::holds_alternative<int>(value)) {
      ss << std::get<int>(value);
    } else if (std::holds_alternative<std::string>(value)) {
      ss << std::get<std::string>(value);
    }

    if (histories.find(name) != histories.end()) {
      graph_names.push_back(name);
      const auto& h = histories[name];
      ss << " [" << std::fixed << std::setprecision(1) << h.min_value << " - "
         << h.max_value << "]";
    }
    lines.push_back(ss.str());
  }

  float max_width = 250.0f;
  for (const auto& line : lines) {
    const float width = renderer.MeasureText(line, kFontSize).x;
    max_width = std::max(max_width, width);
  }

  const float line_height = kFontSize + kLineSpacing;
  float content_height =
      static_cast<float>(lines.size()) * line_height + kPadding * 2.0f;

  content_height += graph_names.size() * (kGraphHeight + kPadding);

  const float box_width = max_width + kPadding * 2.0f;
  const float origin_x =
      std::max(kCornerOffset,
               static_cast<float>(window_size.x) - box_width - kCornerOffset);
  const float origin_y = kCornerOffset;

  const math::RectF background{origin_x, origin_y, box_width, content_height};
  renderer.DrawRect(background, kBackgroundColor);

  float y = background.top_left_y_ + kPadding;
  const float x = background.top_left_x_ + kPadding;

  for (const auto& line : lines) {
    renderer.DrawText(line, {x, y}, kFontSize, kTextColor);
    y += line_height;
  }

  for (const auto& name : graph_names) {
    y += kPadding;
    DrawGraph(renderer, name, histories[name], x, y, max_width);
    y += kGraphHeight;
  }
}

void ProfilerOverlay::DrawGraph(render::Renderer2D& renderer,
                                const std::string& name,
                                const MetricHistory& history, float x, float& y,
                                float width) {
  math::RectF bg{x, y, width, kGraphHeight};
  renderer.DrawRect(bg, kGraphBgColor);

  if (history.values.empty()) return;

  float range = history.max_value - history.min_value;
  if (range <= 0.0001f) range = 1.0f;

  float step = width / static_cast<float>(history.max_samples);

  for (size_t i = 1; i < history.values.size(); ++i) {
    float v1 = history.values[i - 1];
    float v2 = history.values[i];

    float h1 = ((v1 - history.min_value) / range) * kGraphHeight;
    float h2 = ((v2 - history.min_value) / range) * kGraphHeight;

    h1 = std::clamp(h1, 0.0f, kGraphHeight);
    h2 = std::clamp(h2, 0.0f, kGraphHeight);

    math::Vector2f p1{x + (i - 1) * step, y + kGraphHeight - h1};
    math::Vector2f p2{x + i * step, y + kGraphHeight - h2};

    math::RectF bar{x + i * step, y + kGraphHeight - h2, step, h2};
    renderer.DrawRect(bar, kGraphColor);
  }
}

}  // namespace engine::profiler
