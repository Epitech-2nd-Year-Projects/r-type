#include "engine/math/transform.h"

namespace engine::math {

Transform::Transform() : position_(0, 0), rotation_(0.0f), scale_(1, 1) {}

Transform::Transform(const Vector2f& pos)
    : position_(pos), rotation_(0.0f), scale_(1, 1) {}

Transform::Transform(const Vector2f& pos, float rot, const Vector2f& scl)
    : position_(pos), rotation_(rot), scale_(scl) {}

Vector2f Transform::GetPosition() const { return position_; }

void Transform::SetPosition(const Vector2f& pos) { position_ = pos; }

float Transform::GetRotation() const { return rotation_; }

void Transform::SetRotation(float degrees) { rotation_ = degrees; }

void Transform::Rotate(float degrees) { rotation_ += degrees; }

void Transform::RotateRad(float radians) {
  rotation_ += radians * 180.0f / 3.14159265359f;
}

float Transform::GetRotationRad() const {
  return rotation_ * 3.14159265359f / 180.0f;
}

void Transform::SetRotationRad(float rad) {
  rotation_ = rad * 180.0f / 3.14159265359f;
}

Vector2f Transform::GetScale() const { return scale_; }

void Transform::SetScale(const Vector2f& scl) { scale_ = scl; }

}  // namespace engine::math