#ifndef ENGINE_RENDER_WINDOW_H_
#define ENGINE_RENDER_WINDOW_H_

#include <memory>
#include <string>
#include <string_view>

#include "context.h"
#include "engine/math/vector2.h"

namespace engine::input {
class InputManager;
}

namespace engine::render {

/**
 * @brief Parameters for creating a rendering window.
 */
struct WindowConfig {
  math::Vector2i size{1280, 720};
  std::string title{"Engine"};
  bool fullscreen{false};
  bool resizable{true};
  bool vsync{true};
  int target_fps{60};
  /**
   * @brief Optional input manager that receives translated window events
   */
  std::shared_ptr<input::InputManager> input_manager{};
};

/**
 * @brief Abstract window surface owning a render context.
 */
class Window {
 public:
  virtual ~Window() = default;

  virtual void PollEvents() = 0;
  virtual bool ShouldClose() const = 0;
  virtual void RequestClose() = 0;
  /**
   * @brief Register an input manager sink for window input events
   */
  virtual void SetInputManager(
      std::shared_ptr<input::InputManager> input_manager) = 0;

  virtual math::Vector2i GetSize() const = 0;
  virtual void SetSize(const math::Vector2i& size) = 0;
  virtual void SetTitle(std::string_view title) = 0;

  /**
   * @brief Time elapsed since last frame in seconds.
   */
  virtual float GetFrameTime() const = 0;

  /**
   * @brief Access the rendering context.
   */
  virtual RenderContext& GetRenderContext() = 0;
};

}  // namespace engine::render

#endif  // ENGINE_RENDER_WINDOW_H_
