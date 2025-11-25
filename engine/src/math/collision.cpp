#include "engine/math/collision.h"

#include <algorithm>
#include <cmath>

namespace engine::math {

bool Collision::AABBVsAABB(const RectF& a, const RectF& b) {
  return a.Intersects(b);
}

CollisionInfo Collision::AABBCollision(const RectF& a, const RectF& b) {
  CollisionInfo info;
  if (!a.Intersects(b)) return info;
  info.colliding_ = true;
  RectF overlap = a.Intersection(b);
  float overlap_w = overlap.width_;
  float overlap_h = overlap.height_;

  if (overlap_w < overlap_h) {
    if (a.top_left_x_ + a.width_ / 2 < b.top_left_x_ + b.width_ / 2) {
      info.normal_ = Vector2f(-1, 0);
    } else {
      info.normal_ = Vector2f(1, 0);
    }
    info.penetration_ = overlap_w;
  } else {
    if (a.top_left_y_ + a.height_ / 2 < b.top_left_y_ + b.height_ / 2) {
      info.normal_ = Vector2f(0, -1);
    } else {
      info.normal_ = Vector2f(0, 1);
    }
    info.penetration_ = overlap_h;
  }
  return info;
}

bool Collision::CircleVsCircle(const Vector2f& center_a, float radius_a,
                               const Vector2f& center_b, float radius_b) {
  float dist = center_a.Distance(center_b);
  float radii_sum = radius_a + radius_b;
  return dist <= radii_sum;
}

bool Collision::AABBVsCircle(const RectF& rect, const Vector2f& center,
                             float radius) {
  float cx = std::max(rect.top_left_x_,
                      std::min(center.x, rect.top_left_x_ + rect.width_));
  float cy = std::max(rect.top_left_y_,
                      std::min(center.y, rect.top_left_y_ + rect.height_));
  Vector2f closest(cx, cy);
  float dist = center.Distance(closest);
  return dist <= radius;
}

bool Collision::PointInCircle(const Vector2f& point, const Vector2f& center,
                              float radius) {
  return point.Distance(center) <= radius;
}

bool Collision::PointInRect(const Vector2f& point, const RectF& rect) {
  return rect.Contains(point);
}

bool Collision::RayVsAABB(const Vector2f& ray_origin, const Vector2f& ray_dir,
                          const RectF& rect, float& out_t) {
  float tmin = -1e9f;
  float tmax = 1e9f;
  if (std::abs(ray_dir.x) > 0.0001f) {
    float tx1 = (rect.top_left_x_ - ray_origin.x) / ray_dir.x;
    float tx2 = (rect.top_left_x_ + rect.width_ - ray_origin.x) / ray_dir.x;
    if (tx1 > tx2) std::swap(tx1, tx2);
    tmin = std::max(tmin, tx1);
    tmax = std::min(tmax, tx2);
  } else {
    if (ray_origin.x < rect.top_left_x_ ||
        ray_origin.x > rect.top_left_x_ + rect.width_) {
      return false;
    }
  }

  if (std::abs(ray_dir.y) > 0.0001f) {
    float ty1 = (rect.top_left_y_ - ray_origin.y) / ray_dir.y;
    float ty2 = (rect.top_left_y_ + rect.height_ - ray_origin.y) / ray_dir.y;
    if (ty1 > ty2) std::swap(ty1, ty2);
    tmin = std::max(tmin, ty1);
    tmax = std::min(tmax, ty2);
  } else {
    if (ray_origin.y < rect.top_left_y_ ||
        ray_origin.y > rect.top_left_y_ + rect.height_) {
      return false;
    }
  }

  if (tmin > tmax || tmax < 0.0f) return false;

  out_t = tmin >= 0.0f ? tmin : tmax;
  return true;
}

bool Collision::RayVsCircle(const Vector2f& ray_origin, const Vector2f& ray_dir,
                            const Vector2f& center, float radius,
                            float& out_t) {
  Vector2f oc = ray_origin - center;
  float a = ray_dir.Dot(ray_dir);
  float b = 2.0f * oc.Dot(ray_dir);
  float c = oc.Dot(oc) - radius * radius;
  float discriminant = b * b - 4 * a * c;

  if (discriminant < 0.0f) return false;

  float sqrt_disc = std::sqrt(discriminant);
  float t1 = (-b - sqrt_disc) / (2 * a);
  float t2 = (-b + sqrt_disc) / (2 * a);

  if (t1 >= 0.0f) {
    out_t = t1;
    return true;
  }
  if (t2 >= 0.0f) {
    out_t = t2;
    return true;
  }
  return false;
}

}  // namespace engine::math