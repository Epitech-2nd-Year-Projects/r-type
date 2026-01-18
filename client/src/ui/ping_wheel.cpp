#include "ping_wheel.h"

#include <cmath>
#include <numbers>

namespace client::ui {

namespace {
// Helper to create color from bytes
engine::render::Color FromBytes(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) {
    return engine::render::Color::FromBytes(r, g, b, a);
}
}  // namespace

PingWheel::PingWheel()
    : options_{
          {protocol::PingType::kAttack, "Attack", FromBytes(255, 80, 80)},
          {protocol::PingType::kDefend, "Defend", FromBytes(80, 120, 255)},
          {protocol::PingType::kDanger, "Danger", FromBytes(255, 180, 50)},
          {protocol::PingType::kOnMyWay, "On My Way", FromBytes(80, 255, 120)},
          {protocol::PingType::kGeneric, "Here", FromBytes(200, 200, 220)}
      } {}

void PingWheel::Update(engine::input::InputManager& input, const engine::math::Vector2i& window_size) {
    auto mouse_pos = input.GetMousePosition();
    
    // Set center position to cursor on first activation
    if (!active_) {
        center_pos_ = mouse_pos;
        active_ = true;
    }

    // Calculate selection based on mouse position relative to center
    engine::math::Vector2f mouse_vec = {mouse_pos.x - center_pos_.x, mouse_pos.y - center_pos_.y};
    
    float len_sq = mouse_vec.x * mouse_vec.x + mouse_vec.y * mouse_vec.y;
    if (len_sq < kInnerRadius * kInnerRadius) {
        selection_ = std::nullopt;
        return;
    }

    float angle = std::atan2(mouse_vec.y, mouse_vec.x); 
    if (angle < 0) angle += 2.0f * std::numbers::pi_v<float>;
    
    float step = 2.0f * std::numbers::pi_v<float> / static_cast<float>(options_.size());
    float shifted_angle = angle + step / 2.0f;
    if (shifted_angle >= 2.0f * std::numbers::pi_v<float>) shifted_angle -= 2.0f * std::numbers::pi_v<float>;
    
    int index = static_cast<int>(shifted_angle / step);
    if (index >= 0 && index < static_cast<int>(options_.size())) {
        selection_ = options_[index].type;
    } else {
        selection_ = std::nullopt;
    }
}

void PingWheel::Draw(engine::render::Renderer2D& renderer) {
    if (options_.empty()) return;

    float step_deg = 360.0f / static_cast<float>(options_.size());
    float step_rad = 2.0f * std::numbers::pi_v<float> / static_cast<float>(options_.size());

    // Outer glow rings
    renderer.DrawRing(center_pos_, kWheelRadius, kWheelRadius + 8.0f, 0, 360, 64, FromBytes(100, 180, 255, 50));
    renderer.DrawRing(center_pos_, kWheelRadius + 4.0f, kWheelRadius + 14.0f, 0, 360, 64, FromBytes(100, 180, 255, 25));

    // Draw each sector
    for (size_t i = 0; i < options_.size(); ++i) {
        const auto& opt = options_[i];
        bool is_selected = (selection_ == opt.type);
        
        float start_angle = static_cast<float>(i) * step_deg - step_deg / 2.0f;
        float end_angle = start_angle + step_deg;
        
        // Sector color - dimmed base, bright when selected
        engine::render::Color sector_color = {
            opt.color.r * 0.35f,
            opt.color.g * 0.35f,
            opt.color.b * 0.35f,
            0.75f
        };
        
        if (is_selected) {
            sector_color = {opt.color.r, opt.color.g, opt.color.b, 0.9f};
            // Glow effect for selected
            renderer.DrawRing(center_pos_, kWheelRadius - 2.0f, kWheelRadius + 8.0f, 
                start_angle, end_angle, 48, opt.color.WithAlpha(0.5f));
        }
        
        renderer.DrawRing(center_pos_, kInnerRadius + 3.0f, kWheelRadius - 2.0f, 
            start_angle, end_angle, 48, sector_color);
    }
    
    // Sector divider lines
    for (size_t i = 0; i < options_.size(); ++i) {
        float rad = (static_cast<float>(i) * step_deg - step_deg / 2.0f) * (std::numbers::pi_v<float> / 180.0f);
        engine::math::Vector2f line_start = {
            center_pos_.x + std::cos(rad) * (kInnerRadius + 5.0f),
            center_pos_.y + std::sin(rad) * (kInnerRadius + 5.0f)
        };
        engine::math::Vector2f line_end = {
            center_pos_.x + std::cos(rad) * (kWheelRadius - 4.0f),
            center_pos_.y + std::sin(rad) * (kWheelRadius - 4.0f)
        };
        renderer.DrawLine(line_start, line_end, 2.0f, FromBytes(255, 255, 255, 100));
    }
    
    // Center dark circle
    renderer.DrawCircle(center_pos_, kInnerRadius + 3.0f, FromBytes(15, 18, 25, 250));
    renderer.DrawCircle(center_pos_, kInnerRadius - 2.0f, FromBytes(25, 30, 40, 255));
    
    // Ring borders
    renderer.DrawRing(center_pos_, kWheelRadius - 2.0f, kWheelRadius, 0, 360, 64, FromBytes(255, 255, 255, 120));
    renderer.DrawRing(center_pos_, kInnerRadius, kInnerRadius + 3.0f, 0, 360, 64, FromBytes(255, 255, 255, 100));

    // Labels and icons
    for (size_t i = 0; i < options_.size(); ++i) {
        const auto& opt = options_[i];
        bool is_selected = (selection_ == opt.type);
        
        float mid_angle_rad = static_cast<float>(i) * step_rad;
        float label_radius = (kInnerRadius + kWheelRadius) / 2.0f;
        
        engine::math::Vector2f label_pos = {
            center_pos_.x + std::cos(mid_angle_rad) * label_radius,
            center_pos_.y + std::sin(mid_angle_rad) * label_radius
        };
        
        // Color indicator dot
        float dot_radius = is_selected ? 7.0f : 5.0f;
        engine::math::Vector2f dot_pos = {
            center_pos_.x + std::cos(mid_angle_rad) * (label_radius - 25.0f),
            center_pos_.y + std::sin(mid_angle_rad) * (label_radius - 25.0f)
        };
        
        // Dot glow when selected
        if (is_selected) {
            renderer.DrawCircle(dot_pos, dot_radius + 3.0f, opt.color.WithAlpha(0.4f));
        }
        renderer.DrawCircle(dot_pos, dot_radius, opt.color);
        
        // Text
        float font_size = is_selected ? 18.0f : 14.0f;
        auto text_size = renderer.MeasureText(opt.label, font_size);
        engine::math::Vector2f text_pos = {
            label_pos.x - text_size.x / 2.0f,
            label_pos.y - text_size.y / 2.0f + 10.0f
        };
        
        engine::render::Color text_color = is_selected 
            ? engine::render::Color::White()
            : FromBytes(180, 180, 190, 220);
        
        renderer.DrawText(opt.label, text_pos, font_size, text_color);
    }
    
    // Center crosshair indicator
    renderer.DrawCircle(center_pos_, 6.0f, FromBytes(100, 180, 255, 220));
    renderer.DrawCircle(center_pos_, 3.0f, FromBytes(200, 230, 255, 255));
}

std::optional<protocol::PingType> PingWheel::CommitSelection() {
    active_ = false;
    return selection_;
}

} // namespace client::ui
