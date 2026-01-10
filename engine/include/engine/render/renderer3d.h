#ifndef ENGINE_RENDER_RENDERER3D_H_
#define ENGINE_RENDER_RENDERER3D_H_

#include <memory>
#include <string>

#include "camera3d.h"
#include "color.h"
#include "engine/math/vector2.h"
#include "engine/math/vector3.h"
#include "light.h"
#include "model.h"

namespace engine::render {

/**
 * @brief Parameters for drawing a 3D model.
 */
struct ModelDrawParams {
  /// @brief World position.
  math::Vector3f position{};

  /// @brief Axis to rotate around.
  math::Vector3f rotation_axis{0.0f, 1.0f, 0.0f};

  /// @brief Rotation angle in degrees.
  float rotation_angle{0.0f};

  /// @brief Scale factor per axis.
  math::Vector3f scale{1.0f, 1.0f, 1.0f};

  /// @brief Color tint applied to the model.
  Color tint{Color::White()};
};

/**
 * @brief Generic 3D renderer API.
 *
 * Allows drawing 3D primitives and models independently from the
 * underlying graphics library.
 */
class Renderer3D {
 public:
  virtual ~Renderer3D() = default;

  // === Camera Management ===

  /**
   * @brief Begin 3D rendering mode with the specified camera.
   *
   * Must be called before any 3D draw calls.
   * @param camera The camera to use for the 3D scene.
   */
  virtual void Begin3D(const Camera3D& camera) = 0;

  /**
   * @brief End 3D rendering mode and return to 2D.
   *
   * Must be called after all 3D draw calls are complete.
   */
  virtual void End3D() = 0;

  // === Primitives ===

  /**
   * @brief Draw a solid cube.
   * @param position Center position of the cube.
   * @param size Size in each dimension (width, height, depth).
   * @param color Fill color.
   */
  virtual void DrawCube(const math::Vector3f& position,
                        const math::Vector3f& size, const Color& color) = 0;

  /**
   * @brief Draw a wireframe cube.
   * @param position Center position of the cube.
   * @param size Size in each dimension (width, height, depth).
   * @param color Wire color.
   */
  virtual void DrawCubeWires(const math::Vector3f& position,
                             const math::Vector3f& size,
                             const Color& color) = 0;

  /**
   * @brief Draw a solid sphere.
   * @param center Center position of the sphere.
   * @param radius Sphere radius.
   * @param color Fill color.
   */
  virtual void DrawSphere(const math::Vector3f& center, float radius,
                          const Color& color) = 0;

  /**
   * @brief Draw a wireframe sphere.
   * @param center Center position of the sphere.
   * @param radius Sphere radius.
   * @param rings Number of horizontal rings.
   * @param slices Number of vertical slices.
   * @param color Wire color.
   */
  virtual void DrawSphereWires(const math::Vector3f& center, float radius,
                               int rings, int slices, const Color& color) = 0;

  /**
   * @brief Draw a solid cylinder.
   * @param position Base center position.
   * @param radius_top Radius at the top.
   * @param radius_bottom Radius at the bottom.
   * @param height Cylinder height.
   * @param slices Number of side faces.
   * @param color Fill color.
   */
  virtual void DrawCylinder(const math::Vector3f& position, float radius_top,
                            float radius_bottom, float height, int slices,
                            const Color& color) = 0;

  /**
   * @brief Draw a wireframe cylinder.
   * @param position Base center position.
   * @param radius_top Radius at the top.
   * @param radius_bottom Radius at the bottom.
   * @param height Cylinder height.
   * @param slices Number of side faces.
   * @param color Wire color.
   */
  virtual void DrawCylinderWires(const math::Vector3f& position,
                                 float radius_top, float radius_bottom,
                                 float height, int slices,
                                 const Color& color) = 0;

  /**
   * @brief Draw a horizontal plane (ground).
   * @param center Center position of the plane.
   * @param size Size in X and Z dimensions.
   * @param color Fill color.
   */
  virtual void DrawPlane(const math::Vector3f& center,
                         const math::Vector2f& size, const Color& color) = 0;

  /**
   * @brief Draw a reference grid on the XZ plane.
   * @param slices Number of grid divisions.
   * @param spacing Distance between grid lines.
   */
  virtual void DrawGrid(int slices, float spacing) = 0;

  /**
   * @brief Draw a line in 3D space.
   * @param start Start point.
   * @param end End point.
   * @param color Line color.
   */
  virtual void DrawLine3D(const math::Vector3f& start,
                          const math::Vector3f& end, const Color& color) = 0;

  /**
   * @brief Draw a point in 3D space.
   * @param position Point position.
   * @param color Point color.
   */
  virtual void DrawPoint3D(const math::Vector3f& position,
                           const Color& color) = 0;

  // === Models ===

  /**
   * @brief Draw a 3D model with the specified parameters.
   * @param model The model to draw.
   * @param params Draw parameters (position, rotation, scale, tint).
   */
  virtual void DrawModel(const Model& model, const ModelDrawParams& params) = 0;

  /**
   * @brief Draw a 3D model in wireframe mode.
   * @param model The model to draw.
   * @param params Draw parameters (position, rotation, scale, tint).
   */
  virtual void DrawModelWires(const Model& model,
                              const ModelDrawParams& params) = 0;

  /**
   * @brief Load a 3D model from file.
   *
   * Supports common formats: .obj, .gltf, .glb, .iqm, .vox
   * @param path Path to the model file.
   * @return Shared pointer to the loaded model, or nullptr on failure.
   */
  virtual std::shared_ptr<Model> LoadModelFromFile(const std::string& path) = 0;

  // === Lighting ===

  /**
   * @brief Set the lighting configuration for subsequent draws.
   *
   * Note: Basic implementation uses Raylib's built-in lighting.
   * @param config The lighting configuration to apply.
   */
  virtual void SetLighting(const LightingConfig& config) = 0;

  /**
   * @brief Disable lighting (use flat colors).
   */
  virtual void DisableLighting() = 0;
};

}  // namespace engine::render

#endif  // ENGINE_RENDER_RENDERER3D_H_
