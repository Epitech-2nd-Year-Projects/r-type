/**
 * @file menu_background_h
 * @brief Menu background video
 *
 * @details
 * Keeps a shared looping background video for menu scenes
 */

#ifndef CLIENT_UI_MENU_BACKGROUND_H_
#define CLIENT_UI_MENU_BACKGROUND_H_

#include <raymedia.h>

#include <string>
#include <string_view>

#include "engine/math/vector2.h"
#include "engine/time/time_delta.h"

namespace client::ui {

/**
 * @brief Shared menu background video
 */
class MenuBackground {
 public:
  /**
   * @brief Create menu background
   * @param video_path Background video path
   */
  explicit MenuBackground(std::string_view video_path);

  /**
   * @brief Destroy menu background
   */
  ~MenuBackground();

  MenuBackground(const MenuBackground&) = delete;
  MenuBackground& operator=(const MenuBackground&) = delete;
  MenuBackground(MenuBackground&&) = delete;
  MenuBackground& operator=(MenuBackground&&) = delete;

  /**
   * @brief Update background playback
   * @param dt Frame time delta
   */
  void Update(engine::time::TimeDelta dt);

  /**
   * @brief Draw background video
   * @param size Render size in pixels
   */
  void Draw(const engine::math::Vector2i& size) const;
  /**
   * @brief Draw background video with alpha
   * @param size Render size in pixels
   * @param alpha Opacity from 0 to 1
   */
  void Draw(const engine::math::Vector2i& size, float alpha) const;

 private:
  std::string video_path_;
  MediaStream media_{};
  bool media_loaded_{false};
};

}  // namespace client::ui

#endif  // CLIENT_UI_MENU_BACKGROUND_H_
