/**
 * @file menu_effects_h
 * @brief Shared menu hover and pointer effects
 *
 * @details
 * Handles pointer animation and menu sound effects for button lists
 */

#ifndef CLIENT_UI_MENU_EFFECTS_H_
#define CLIENT_UI_MENU_EFFECTS_H_

#include <functional>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "engine/input.h"
#include "engine/render/renderer2d.h"
#include "engine/time/time_delta.h"
#include "engine/ui/button.h"

namespace client {
class ClientContext;
}

namespace client::ui {

/**
 * @brief Pointer animation settings
 */
struct MenuPointerConfig {
  /// @brief Pointer frame prefix path
  std::string_view frame_prefix;
  /// @brief Pointer frame extension
  std::string_view frame_extension;
  /// @brief Pointer frame count
  int frame_count{0};
  /// @brief Frame duration in seconds
  float frame_duration{0.0f};
  /// @brief Height factor relative to button height
  float height_factor{1.0f};
  /// @brief Pointer spacing from button
  float spacing{0.0f};
  /// @brief Additional scale multiplier
  float scale_factor{1.0f};
};

/**
 * @brief Menu hover and pointer animation helper
 */
class MenuEffects {
 public:
  /**
   * @brief Create menu effects helper
   * @param context Client context reference
   * @param config Pointer animation settings
   * @param hover_sfx_path Hover sound effect path
   * @param click_sfx_path Click sound effect path
   */
  MenuEffects(ClientContext& context, MenuPointerConfig config,
              std::string_view hover_sfx_path,
              std::string_view click_sfx_path);

  /**
   * @brief Load pointer animation textures
   */
  void Load();

  /**
   * @brief Update hover and animation state
   * @param dt Frame time delta
   * @param input Input manager reference
   * @param buttons Button list
   */
  void Update(engine::time::TimeDelta dt, engine::input::InputManager& input,
              std::span<const std::shared_ptr<engine::ui::Button>> buttons);

  /**
   * @brief Draw pointer animations
   * @param renderer Renderer instance
   * @param buttons Button list
   */
  void DrawPointers(
      engine::render::Renderer2D& renderer,
      std::span<const std::shared_ptr<engine::ui::Button>> buttons) const;

  /**
   * @brief Wrap a click handler with sound effect
   * @param action Action callback
   * @return Wrapped callback
   */
  std::function<void()> WrapClick(std::function<void()> action) const;

 private:
  struct PointerState {
    bool hovered{false};
    bool was_hovered{false};
    float elapsed{0.0f};
    bool animating{false};
  };

  void PlaySound(const std::string& path) const;
  void EnsureStateCount(std::size_t count);

  ClientContext& context_;
  MenuPointerConfig config_{};
  std::vector<std::shared_ptr<engine::render::Texture2D>> pointer_frames_{};
  std::vector<PointerState> pointer_states_{};
  std::string hover_sfx_path_{};
  std::string click_sfx_path_{};
};

}  // namespace client::ui

#endif  // CLIENT_UI_MENU_EFFECTS_H_
