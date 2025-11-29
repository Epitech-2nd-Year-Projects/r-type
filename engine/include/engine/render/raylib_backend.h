#ifndef ENGINE_RENDER_RAYLIB_BACKEND_H_
#define ENGINE_RENDER_RAYLIB_BACKEND_H_

#include <memory>

#include "backend.h"

namespace engine::render {

/**
 * @brief Factory for the Raylib-based rendering backend.
 */
std::unique_ptr<WindowBackend> CreateRaylibBackend();

}  // namespace engine::render

#endif  // ENGINE_RENDER_RAYLIB_BACKEND_H_
