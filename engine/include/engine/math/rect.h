#ifndef ENGINE_MATH_RECT_H_
#define ENGINE_MATH_RECT_H_

#include "vector2.h"

namespace engine::math {

/**
 * @brief Axis-Aligned Bounding Box (AABB).
 * @tparam T Scalar type
 */
template <typename T = float>
class Rect {
 public:
  T top_left_x_, top_left_y_;
  T width_, height_;

  Rect() : top_left_x_(0), top_left_y_(0), width_(0), height_(0) {}

  Rect(T x, T y, T w, T h)
      : top_left_x_(x), top_left_y_(y), width_(w), height_(h) {}

  Rect(const ::engine::math::Vector2<T>& pos,
       const ::engine::math::Vector2<T>& size)
      : top_left_x_(pos.x),
        top_left_y_(pos.y),
        width_(size.x),
        height_(size.y) {}

  /**
   * @brief Get top-left corner.
   */
  ::engine::math::Vector2<T> TopLeft() const {
    return ::engine::math::Vector2<T>(top_left_x_, top_left_y_);
  }

  /**
   * @brief Get top-right corner.
   */
  ::engine::math::Vector2<T> TopRight() const {
    return ::engine::math::Vector2<T>(top_left_x_ + width_, top_left_y_);
  }

  /**
   * @brief Get bottom-left corner.
   */
  ::engine::math::Vector2<T> BottomLeft() const {
    return ::engine::math::Vector2<T>(top_left_x_, top_left_y_ + height_);
  }

  /**
   * @brief Get bottom-right corner.
   */
  ::engine::math::Vector2<T> BottomRight() const {
    return ::engine::math::Vector2<T>(top_left_x_ + width_,
                                      top_left_y_ + height_);
  }

  /**
   * @brief Get center point.
   */
  ::engine::math::Vector2<T> Center() const {
    return ::engine::math::Vector2<T>(top_left_x_ + width_ / 2,
                                      top_left_y_ + height_ / 2);
  }

  /**
   * @brief Test if point is inside rectangle.
   */
  bool Contains(const ::engine::math::Vector2<T>& point) const {
    return point.x >= top_left_x_ && point.x < top_left_x_ + width_ &&
           point.y >= top_left_y_ && point.y < top_left_y_ + height_;
  }

  /**
   * @brief Test AABB intersection with another rectangle.
   */
  bool Intersects(const Rect& other) const {
    return !(top_left_x_ + width_ < other.top_left_x_ ||
             top_left_x_ > other.top_left_x_ + other.width_ ||
             top_left_y_ + height_ < other.top_left_y_ ||
             top_left_y_ > other.top_left_y_ + other.height_);
  }

  /**
   * @brief Get intersection rectangle.
   */
  Rect Intersection(const Rect& other) const {
    if (!Intersects(other)) return Rect();

    T ix = std::max(top_left_x_, other.top_left_x_);
    T iy = std::max(top_left_y_, other.top_left_y_);
    T iw =
        std::min(top_left_x_ + width_, other.top_left_x_ + other.width_) - ix;
    T ih =
        std::min(top_left_y_ + height_, other.top_left_y_ + other.height_) - iy;

    return Rect(ix, iy, iw, ih);
  }

  /**
   * @brief Expand rectangle by margin.
   */
  void Expand(T margin) {
    top_left_x_ -= margin;
    top_left_y_ -= margin;
    width_ += 2 * margin;
    height_ += 2 * margin;
  }

  /**
   * @brief Translate rectangle.
   */
  void Translate(const ::engine::math::Vector2<T>& offset) {
    top_left_x_ += offset.x;
    top_left_y_ += offset.y;
  }

  /**
   * @brief Scale rectangle from center.
   */
  void Scale(T factor) {
    ::engine::math::Vector2<T> c = Center();
    width_ *= factor;
    height_ *= factor;
    top_left_x_ = c.x - width_ / 2;
    top_left_y_ = c.y - height_ / 2;
  }
};

using RectF = Rect<float>;
using RectD = Rect<double>;
using RectI = Rect<int>;

}  // namespace engine::math

#endif