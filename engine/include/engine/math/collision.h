#ifndef ENGINE_MATH_COLLISION_H_
#define ENGINE_MATH_COLLISION_H_

#include "rect.h"
#include "vector2.h"

namespace engine::math {

/**
 * @brief Collision query results.
 */
struct CollisionInfo {
  bool colliding_ = false;
  Vector2f normal_{0, 0};
  float penetration_ = 0.0f;
};

/**
 * @brief Static collision detection utilities.
 */
class Collision {
 public:
  /**
   * @brief Test AABB vs AABB collision.
   */
  static bool AABBVsAABB(const RectF& a, const RectF& b);

  /**
   * @brief AABB vs AABB with collision info.
   */
  static CollisionInfo AABBCollision(const RectF& a, const RectF& b);

  /**
   * @brief Circle vs circle collision.
   */
  static bool CircleVsCircle(const Vector2f& center_a, float radius_a,
                             const Vector2f& center_b, float radius_b);

  /**
   * @brief AABB vs circle collision.
   */
  static bool AABBVsCircle(const RectF& rect, const Vector2f& center,
                           float radius);

  /**
   * @brief Point in circle test.
   */
  static bool PointInCircle(const Vector2f& point, const Vector2f& center,
                            float radius);

  /**
   * @brief Point in rect test.
   */
  static bool PointInRect(const Vector2f& point, const RectF& rect);

  /**
   * @brief Ray vs AABB intersection.
   */
  static bool RayVsAABB(const Vector2f& ray_origin, const Vector2f& ray_dir,
                        const RectF& rect, float& out_t);

  /**
   * @brief Ray vs circle intersection.
   */
  static bool RayVsCircle(const Vector2f& ray_origin, const Vector2f& ray_dir,
                          const Vector2f& center, float radius, float& out_t);
};

}  // namespace engine::math

#endif  // ENGINE_MATH_COLLISION_H_