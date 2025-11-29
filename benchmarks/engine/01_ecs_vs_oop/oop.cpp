#include "oop.h"

void Player::UpdatePhysics(float dt) {
  x_ += vx_ * dt;
  y_ += vy_ * dt;
}

void Player::UpdateHealth(float dt) {
  health_--;
  if (health_ < 0) health_ = 0;
}

void Player::UpdateAI(float dt) {
  ai_timer_ += dt;
  if (ai_timer_ >= 0.05f) {
    direction_ = -direction_;
    vx_ = 1.0f * direction_;
    vy_ = 1.5f * direction_;
    ai_timer_ = 0.0f;
  }
}

void Player::UpdateDOT(float dt) {
  dot_timer_ += dt;
  if (dot_timer_ >= dot_interval_) {
    health_ -= dot_damage_;
    if (health_ < 0) health_ = 0;
    dot_timer_ = 0.0f;
  }
}

void Player::Serialize(SerializedEntity &out) const {
  out.x = x_;
  out.y = y_;
  out.vx = vx_;
  out.vy = vy_;
  out.hp = health_;
}

void Enemy::UpdatePhysics(float dt) {
  x_ += vx_ * dt;
  y_ += vy_ * dt;
}

void Enemy::UpdateHealth(float dt) {
  health_--;
  if (health_ < 0) health_ = 0;
}

void Enemy::UpdateAI(float dt) {
  ai_timer_ += dt;
  if (ai_timer_ >= 0.07f) {
    direction_ = -direction_;
    vx_ = -1.0f * direction_;
    vy_ = 0.5f * direction_;
    ai_timer_ = 0.0f;
  }
}

void Enemy::UpdateDOT(float dt) {
  dot_timer_ += dt;
  if (dot_timer_ >= dot_interval_) {
    health_ -= dot_damage_;
    if (health_ < 0) health_ = 0;
    dot_timer_ = 0.0f;
  }
}

void Enemy::Serialize(SerializedEntity &out) const {
  out.x = x_;
  out.y = y_;
  out.vx = vx_;
  out.vy = vy_;
  out.hp = health_;
}

void OOPGameWorld::AddEntity(std::unique_ptr<GameEntity> entity) {
  entities_.push_back(std::move(entity));
}

void OOPGameWorld::UpdatePhysics(float dt) {
  for (auto &e : entities_) e->UpdatePhysics(dt);
}

void OOPGameWorld::UpdateHealth(float dt) {
  for (auto &e : entities_) e->UpdateHealth(dt);
}

void OOPGameWorld::UpdateAI(float dt) {
  for (auto &e : entities_) e->UpdateAI(dt);
}

void OOPGameWorld::UpdateDOT(float dt) {
  for (auto &e : entities_) e->UpdateDOT(dt);
}

void OOPGameWorld::SerializeAll(std::vector<SerializedEntity> &out) const {
  out.resize(entities_.size());
  for (size_t i = 0; i < entities_.size(); ++i) {
    entities_[i]->Serialize(out[i]);
  }
}

size_t OOPGameWorld::EntityCount() const { return entities_.size(); }
