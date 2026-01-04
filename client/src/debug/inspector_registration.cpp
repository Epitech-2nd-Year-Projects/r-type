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
        }
        return modified;
      });
}

}  // namespace client::debug
