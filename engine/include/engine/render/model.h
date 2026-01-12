#ifndef ENGINE_RENDER_MODEL_H_
#define ENGINE_RENDER_MODEL_H_

#include "engine/math/vector3.h"

namespace engine::render {

/**
 * @brief Handle for GPU 3D model resource.
 *
 * Abstract base class representing a loaded 3D model.
 * Implementations manage the underlying graphics API resources.
 */
class Model {
 public:
  virtual ~Model() = default;

  /**
   * @brief Get the bounding box dimensions of the model.
   * @return Size of the axis-aligned bounding box (width, height, depth).
   */
  virtual math::Vector3f GetBoundingBoxSize() const = 0;

  /**
   * @brief Get the bounding box center offset from origin.
   * @return Center point of the bounding box relative to model origin.
   */
  virtual math::Vector3f GetBoundingBoxCenter() const = 0;
};

}  // namespace engine::render

#endif  // ENGINE_RENDER_MODEL_H_
