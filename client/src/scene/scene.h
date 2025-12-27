#ifndef CLIENT_SCENE_SCENE_H_
#define CLIENT_SCENE_SCENE_H_

#include <memory>

#include "engine/render/renderer2d.h"
#include "engine/time/time_delta.h"

namespace client {

/**
 * @brief Abstract base class for all game screens
 */
class Scene {
 public:
  virtual ~Scene() = default;

  /**
   * @brief Update scene logic
   * @param dt Time delta since last frame
   */
  virtual void Update(engine::time::TimeDelta dt) = 0;

  /**
   * @brief Render scene content
   * @param renderer The 2D renderer to use
   */
  virtual void Draw(engine::render::Renderer2D& renderer) = 0;

  /**
   * @brief Check if the scene is currently capturing text input
   * @return True when text input is active such as typing in a field and global
   * hotkeys should be suppressed
   */
  virtual bool IsInputCaptured() const { return false; }
};

}  // namespace client

#endif  // CLIENT_SCENE_SCENE_H_
