#include "ping_wheel.h"

#include <cmath>
#include <numbers>

namespace client::ui {

PingWheel::PingWheel()
    : options_{
          {protocol::PingType::kAttack, "Attack", {255, 50, 50, 255}},
          {protocol::PingType::kDefend, "Defend", {50, 50, 255, 255}},
          {protocol::PingType::kDanger, "Danger", {255, 165, 0, 255}},
          {protocol::PingType::kOnMyWay, "On My Way", {50, 255, 50, 255}},
          {protocol::PingType::kGeneric, "Here", {200, 200, 200, 255}}
      } {}

void PingWheel::Update(engine::input::InputManager& input, const engine::math::Vector2i& window_size) {
    active_ = true;
    center_pos_ = {static_cast<float>(window_size.x) / 2.0f, static_cast<float>(window_size.y) / 2.0f};

    // Calculate selection based on mouse position relative to center
    auto mouse_pos = input.GetMousePosition();
    engine::math::Vector2f mouse_vec = {mouse_pos.x - center_pos_.x, mouse_pos.y - center_pos_.y};
    
    float len_sq = mouse_vec.x * mouse_vec.x + mouse_vec.y * mouse_vec.y;
    if (len_sq < kInnerRadius * kInnerRadius) {
        selection_ = std::nullopt; // Too close to center (deadzone)
        return;
    }

    // Angle calculation: -PI to PI
    float angle = std::atan2(mouse_vec.y, mouse_vec.x); 
    // Normalize to 0 to 2PI
    if (angle < 0) angle += 2.0f * std::numbers::pi_v<float>;
    
    float step = 2.0f * std::numbers::pi_v<float> / static_cast<float>(options_.size());
    
    // Shift angle by step/2 to center the first sector around 0 (Right)
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

    // Draw background ring
    renderer.DrawCircle(center_pos_, kWheelRadius, {0, 0, 0, 150});
    renderer.DrawCircle(center_pos_, kInnerRadius, {0, 0, 0, 200}); // Inner hole

    float step = 360.0f / options_.size();
    
    for (size_t i = 0; i < options_.size(); ++i) {
        const auto& opt = options_[i];
        
        bool is_selected = (selection_ == opt.type);
        
        float start_angle = i * step - step / 2.0f;
        float end_angle = start_angle + step;
        
        // Convert to radians for position calc
        float mid_angle_rad = (i * step) * (std::numbers::pi_v<float> / 180.0f);
        
        // Highlight sector
        if (is_selected) {
             renderer.DrawRing(center_pos_, kInnerRadius, kWheelRadius, start_angle, end_angle, 48, {opt.color.r, opt.color.g, opt.color.b, 100});
        }
        
        // Draw Sector Borders
        float rad = (i * step - step / 2.0f) * (std::numbers::pi_v<float> / 180.0f);
        engine::math::Vector2f line_end = {
            center_pos_.x + std::cos(rad) * kWheelRadius,
            center_pos_.y + std::sin(rad) * kWheelRadius
        };
        engine::math::Vector2f line_start = {
            center_pos_.x + std::cos(rad) * kInnerRadius,
            center_pos_.y + std::sin(rad) * kInnerRadius
        };
        renderer.DrawLine(line_start, line_end, 1.0f, {255, 255, 255, 50});

        // Draw Text/Label
        float label_radius = (kInnerRadius + kWheelRadius) / 2.0f;
        engine::math::Vector2f label_pos = {
            center_pos_.x + std::cos(mid_angle_rad) * label_radius,
            center_pos_.y + std::sin(mid_angle_rad) * label_radius
        };
        
        // Measure text for proper centering
        auto text_size = renderer.MeasureText(opt.label, 20);
        engine::math::Vector2f text_pos = {
            label_pos.x - text_size.x / 2.0f,
            label_pos.y - text_size.y / 2.0f
        };
        
        renderer.DrawText(opt.label, text_pos, 20, is_selected ? engine::render::Color{255, 255, 255, 255} : engine::render::Color{200, 200, 200, 200});
    }
}

std::optional<protocol::PingType> PingWheel::CommitSelection() {
    active_ = false;
    return selection_;
}

} // namespace client::ui
