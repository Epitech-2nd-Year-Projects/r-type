#ifndef ENGINE_RENDER_CONTEXT_H_
#define ENGINE_RENDER_CONTEXT_H_

#include "color.h"

namespace engine::render {

class Renderer2D;

/**
 * @brief Rendering context representing a frame lifecycle.
 */
class RenderContext {
 public:
  virtual ~RenderContext() = default;

  /**
   * @brief Begin issuing rendering commands.
   */
  virtual void BeginFrame() = 0;

  /**
   * @brief Finish issuing rendering commands and present.
   */
  virtual void EndFrame() = 0;

  /**
   * @brief Clear framebuffer with given color.
   */
  virtual void Clear(const Color& color) = 0;

  /**
   * @brief Access the 2D renderer bound to the context.
   */
  virtual Renderer2D& Get2DRenderer() = 0;
};

}  // namespace engine::render

#endif  // ENGINE_RENDER_CONTEXT_H_
