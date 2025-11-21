#include "oop.h"

void Player::UpdatePhysics(float dt) {
  x_ += vx_ * dt;
  y_ += vy_ * dt;
}

void Player::UpdateHealth(float dt) {
  health_--;
  if (health_ < 0)
    health_ = 0;
}

void Enemy::UpdatePhysics(float dt) {
  x_ += vx_ * dt;
  y_ += vy_ * dt;
}

void Enemy::UpdateHealth(float dt) {
  health_--;
  if (health_ < 0)
    health_ = 0;
}

void OOPGameWorld::AddEntity(std::unique_ptr<GameEntity> entity) {
  entities_.push_back(std::move(entity));
}

void OOPGameWorld::UpdatePhysics(float dt) {
  for (auto &e : entities_)
    e->UpdatePhysics(dt);
}

void OOPGameWorld::UpdateHealth(float dt) {
  for (auto &e : entities_)
    e->UpdateHealth(dt);
}

size_t OOPGameWorld::EntityCount() const { return entities_.size(); }
