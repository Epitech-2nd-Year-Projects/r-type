#ifndef ENGINE_RENDER_BACKEND_H_
#define ENGINE_RENDER_BACKEND_H_

#include <memory>
#include <string_view>

#include "window.h"

namespace engine::render {

/**
 * @brief Abstract factory responsible for creating rendering windows.
 */
class WindowBackend {
 public:
  virtual ~WindowBackend() = default;

  virtual std::string_view Name() const = 0;
  virtual std::unique_ptr<Window> CreateWindow(const WindowConfig& config) = 0;
};

}  // namespace engine::render

#endif  // ENGINE_RENDER_BACKEND_H_
