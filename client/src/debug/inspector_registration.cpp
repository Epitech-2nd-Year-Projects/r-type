#include "debug/inspector_registration.h"

#include <imgui.h>

#include <cstring>
#include <functional>
#include <span>
#include <utility>
#include <vector>

#include "ecs/components.h"
#include "engine/debug/component_inspector_registry.h"
#include "protocol/command.h"

namespace client::debug {

void RegisterClientInspectors(
    engine::debug::ComponentInspectorRegistry& registry, bool is_debug,
    std::function<void(const protocol::CommandPayload&)> send_command) {
  auto params_to_payload = [send_command](
                               engine::ecs::EntityId entity_id,
                               protocol::DebugComponentId component_id,
                               const void* data, std::size_t size) {
    if (!send_command) return;

    protocol::CommandPayload payload;
    payload.command_id = static_cast<std::uint16_t>(
        protocol::CommandType::kDebugUpdateComponent);

    const std::uint32_t id = entity_id;
    const std::uint32_t comp_id = static_cast<std::uint32_t>(component_id);

    payload.payload.resize(sizeof(id) + sizeof(comp_id) + size);
    std::span<char> payload_span(payload.payload);
    auto bytes = std::as_writable_bytes(payload_span);

    std::memcpy(bytes.data(), &id, sizeof(id));
    bytes = bytes.subspan(sizeof(id));
    std::memcpy(bytes.data(), &comp_id, sizeof(comp_id));
    bytes = bytes.subspan(sizeof(comp_id));
    std::memcpy(bytes.data(), data, size);

    send_command(payload);
  };

  registry.Register<ecs::SpriteComponent>(
      "Sprite", [is_debug, params_to_payload](ecs::SpriteComponent& sprite,
                                              engine::ecs::EntityId id) {
        bool modified = false;
        if (ImGui::CollapsingHeader("Sprite", ImGuiTreeNodeFlags_DefaultOpen)) {
          if (!is_debug) {
            ImGui::BeginDisabled();
          }
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
          if (!is_debug) {
            ImGui::EndDisabled();
          }
        }
        return modified;
      });

  registry.Register<ecs::PositionComponent>(
      "Position", [is_debug, params_to_payload](ecs::PositionComponent& pos,
                                                engine::ecs::EntityId id) {
        bool modified = false;
        if (ImGui::CollapsingHeader("Position (Server Auth)",
                                    ImGuiTreeNodeFlags_DefaultOpen)) {
          if (!is_debug) {
            ImGui::BeginDisabled();
          }
          float p[2] = {pos.position.x, pos.position.y};
          if (ImGui::DragFloat2("xy", p, 0.1f)) {
            pos.position.x = p[0];
            pos.position.y = p[1];
            modified = true;
          }
          ImGui::TextDisabled("Render: (%.1f, %.1f)", pos.render_position.x,
                              pos.render_position.y);
          if (!is_debug) {
            ImGui::TextColored(ImVec4(1, 1, 0, 1), "Reset by server!");
          }
          if (!is_debug) {
            ImGui::EndDisabled();
          }
        }
        if (modified && is_debug) {
          // Send only the position vector (8 bytes), skipping the rest of the struct
          params_to_payload(id, protocol::DebugComponentId::kTransform,
                            &pos.position, sizeof(pos.position));
        }
        return modified;
      });

  registry.Register<ecs::VelocityComponent>(
      "Velocity", [is_debug, params_to_payload](ecs::VelocityComponent& vel,
                                                engine::ecs::EntityId id) {
        bool modified = false;
        if (ImGui::CollapsingHeader("Velocity (Server Auth)")) {
          if (!is_debug) {
            ImGui::BeginDisabled();
          }
          float v[2] = {vel.velocity.x, vel.velocity.y};
          if (ImGui::DragFloat2("xy", v, 0.1f)) {
            vel.velocity.x = v[0];
            vel.velocity.y = v[1];
            modified = true;
          }
          if (!is_debug) {
            ImGui::EndDisabled();
          }
        }
        if (modified && is_debug) {
          params_to_payload(id, protocol::DebugComponentId::kVelocity, &vel,
                            sizeof(vel));
        }
        return modified;
      });

  registry.Register<ecs::RenderLayerComponent>(
      "Render Layer",
      [is_debug, params_to_payload](ecs::RenderLayerComponent& layer,
                                    engine::ecs::EntityId id) {
        bool modified = false;
        if (ImGui::CollapsingHeader("Render Layer")) {
          if (!is_debug) {
            ImGui::BeginDisabled();
          }
          if (ImGui::InputInt("Layer", &layer.layer)) {
            modified = true;
          }
          if (ImGui::DragFloat("Depth", &layer.depth, 0.01f, 0.0f, 1.0f)) {
            modified = true;
          }
          if (!is_debug) {
            ImGui::EndDisabled();
          }
        }
        return modified;
      });

  registry.Register<ecs::HealthComponent>(
      "Health", [is_debug, params_to_payload](ecs::HealthComponent& health,
                                              engine::ecs::EntityId id) {
        bool modified = false;
        if (ImGui::CollapsingHeader("Health")) {
          if (!is_debug) {
            ImGui::BeginDisabled();
          }
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
          if (!is_debug) {
            ImGui::TextColored(ImVec4(1, 1, 0, 1),
                               "Note: Reset by server each frame!");
          }
          if (!is_debug) {
            ImGui::EndDisabled();
          }
        }
        if (modified && is_debug) {
          params_to_payload(id, protocol::DebugComponentId::kHealth, &health,
                            sizeof(health));
        }
        return modified;
      });

  registry.Register<ecs::PlayerStateComponent>(
      "Player State",
      [is_debug, params_to_payload](ecs::PlayerStateComponent& state,
                                    engine::ecs::EntityId id) {
        bool modified = false;
        if (ImGui::CollapsingHeader("Player State (Server Auth)")) {
          if (!is_debug) {
            ImGui::BeginDisabled();
          }
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
          if (!is_debug) {
            ImGui::TextColored(ImVec4(1, 1, 0, 1),
                               "Note: Reset by server each frame!");
          }
          if (!is_debug) {
            ImGui::EndDisabled();
          }
        }
        if (modified && is_debug) {
          params_to_payload(id, protocol::DebugComponentId::kPlayerState,
                            &state, sizeof(state));
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
