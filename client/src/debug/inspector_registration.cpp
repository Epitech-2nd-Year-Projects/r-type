#include "debug/inspector_registration.h"

#include <imgui.h>

#include "ecs/components.h"
#include "engine/debug/component_inspector_registry.h"

namespace client::debug {

void RegisterClientInspectors(
    engine::debug::ComponentInspectorRegistry& registry) {
  registry.Register<ecs::SpriteComponent>(
      "Sprite", [](ecs::SpriteComponent& sprite, const engine::ecs::EntityId&) {
        bool modified = false;
        if (ImGui::CollapsingHeader("Sprite", ImGuiTreeNodeFlags_DefaultOpen)) {
          ImGui::Text("Texture ID: %s", sprite.texture_id.c_str());

          if (ImGui::Checkbox("Visible", &sprite.visible)) {
            modified = true;
          }
          if (ImGui::TreeNode("Source Rect")) {
            float rect[4] = {
                sprite.source_rect.top_left_x_, sprite.source_rect.top_left_y_,
                sprite.source_rect.width_, sprite.source_rect.height_};
            if (ImGui::DragFloat4("##rect", rect)) {
              sprite.source_rect.top_left_x_ = rect[0];
              sprite.source_rect.top_left_y_ = rect[1];
              sprite.source_rect.width_ = rect[2];
              sprite.source_rect.height_ = rect[3];
              modified = true;
            }
            ImGui::TreePop();
          }
          ImGui::ColorEdit4("Tint", &sprite.tint.r);
        }
        return modified;
      });

  registry.Register<ecs::PositionComponent>(
      "Position",
      [](ecs::PositionComponent& pos, const engine::ecs::EntityId&) {
        bool modified = false;
        if (ImGui::CollapsingHeader("Position (Server Auth)",
                                    ImGuiTreeNodeFlags_DefaultOpen)) {
          float p[2] = {pos.position.x, pos.position.y};
          if (ImGui::DragFloat2("xy", p, 0.1f)) {
            pos.position.x = p[0];
            pos.position.y = p[1];
            modified = true;
          }
          ImGui::TextDisabled("Render: (%.1f, %.1f)", pos.render_position.x,
                              pos.render_position.y);
          ImGui::TextColored(ImVec4(1, 1, 0, 1), "Reset by server!");
        }
        return modified;
      });

  registry.Register<ecs::VelocityComponent>(
      "Velocity",
      [](ecs::VelocityComponent& vel, const engine::ecs::EntityId&) {
        bool modified = false;
        if (ImGui::CollapsingHeader("Velocity (Server Auth)")) {
          float v[2] = {vel.velocity.x, vel.velocity.y};
          if (ImGui::DragFloat2("xy", v, 0.1f)) {
            vel.velocity.x = v[0];
            vel.velocity.y = v[1];
            modified = true;
          }
        }
        return modified;
      });

  registry.Register<ecs::RenderLayerComponent>(
      "Render Layer",
      [](ecs::RenderLayerComponent& layer, const engine::ecs::EntityId&) {
        bool modified = false;
        if (ImGui::CollapsingHeader("Render Layer")) {
          if (ImGui::InputInt("Layer", &layer.layer)) {
            modified = true;
          }
          if (ImGui::DragFloat("Depth", &layer.depth, 0.01f, 0.0f, 1.0f)) {
            modified = true;
          }
        }
        return modified;
      });

  registry.Register<ecs::HealthComponent>(
      "Health", [](ecs::HealthComponent& health, const engine::ecs::EntityId&) {
        bool modified = false;
        if (ImGui::CollapsingHeader("Health")) {
          int current = static_cast<int>(health.current);
          int max = static_cast<int>(health.max);

          float fraction = (max > 0) ? (float)current / max : 0.0f;
          ImGui::ProgressBar(fraction, ImVec2(0.0f, 0.0f));
          ImGui::SameLine();
          ImGui::Text("%d/%d", current, max);

          if (ImGui::SliderInt("Current (Auth)", &current, 0, 255)) {
            health.current = static_cast<std::uint8_t>(current);
            modified = true;
          }
          if (ImGui::SliderInt("Max (u8 limit)", &max, 0, 255)) {
            health.max = static_cast<std::uint8_t>(max);
            modified = true;
          }
          ImGui::TextColored(ImVec4(1, 1, 0, 1),
                             "Note: Reset by server each frame!");
        }
        return modified;
      });

  registry.Register<ecs::PlayerStateComponent>(
      "Player State",
      [](ecs::PlayerStateComponent& state, const engine::ecs::EntityId&) {
        bool modified = false;
        if (ImGui::CollapsingHeader("Player State (Server Auth)")) {
          ImGui::Text("Player ID: %u", state.player_id);

          int lives = static_cast<int>(state.lives);
          if (ImGui::SliderInt("Lives", &lives, 0, 99)) {
            state.lives = static_cast<std::uint8_t>(lives);
            modified = true;
          }

          int score = static_cast<int>(state.score);
          if (ImGui::InputInt("Score", &score)) {
            state.score = static_cast<std::uint32_t>(score);
            modified = true;
          }
          ImGui::TextColored(ImVec4(1, 1, 0, 1),
                             "Note: Reset by server each frame!");
        }
        return modified;
      });

  registry.Register<ecs::NetworkedEntityComponent>(
      "Network",
      [](ecs::NetworkedEntityComponent& net, const engine::ecs::EntityId&) {
        if (ImGui::CollapsingHeader("Network Info")) {
          ImGui::Text("Net ID: %u", net.network_id);
          ImGui::Text("Type Code: %u", net.type_code);
          ImGui::Text("Last Snapshot: %u", net.last_snapshot);
        }
        return false;
      });
}
}  // namespace client::debug
