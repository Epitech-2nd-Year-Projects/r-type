#ifndef BENCH_OOP_H
#define BENCH_OOP_H

#include <memory>
#include <vector>

#include "bench_types.h"

class GameEntity {
 public:
  virtual ~GameEntity() = default;
  virtual void UpdatePhysics(float dt) = 0;
  virtual void UpdateHealth(float dt) = 0;
  virtual void UpdateAI(float dt) = 0;
  virtual void UpdateDOT(float dt) = 0;
  virtual void Serialize(SerializedEntity &out) const = 0;

 protected:
  float x_ = 0.0f;
  float y_ = 0.0f;
};

class Player : public GameEntity {
 private:
  float vx_ = 1.0f;
  float vy_ = 1.5f;
  int health_ = 100;
  float ai_timer_ = 0.0f;
  float direction_ = 1.0f;
  float dot_timer_ = 0.0f;
  int dot_damage_ = 1;
  float dot_interval_ = 0.1f;

 public:
  void UpdatePhysics(float dt) override;
  void UpdateHealth(float dt) override;
  void UpdateAI(float dt) override;
  void UpdateDOT(float dt) override;
  void Serialize(SerializedEntity &out) const override;
};

class Enemy : public GameEntity {
 private:
  float vx_ = -1.0f;
  float vy_ = 0.5f;
  int health_ = 50;
  float ai_timer_ = 0.0f;
  float direction_ = -1.0f;
  float dot_timer_ = 0.0f;
  int dot_damage_ = 2;
  float dot_interval_ = 0.15f;

 public:
  void UpdatePhysics(float dt) override;
  void UpdateHealth(float dt) override;
  void UpdateAI(float dt) override;
  void UpdateDOT(float dt) override;
  void Serialize(SerializedEntity &out) const override;
};

class OOPGameWorld {
 public:
  void AddEntity(std::unique_ptr<GameEntity> entity);
  void UpdatePhysics(float dt);
  void UpdateHealth(float dt);
  void UpdateAI(float dt);
  void UpdateDOT(float dt);
  void SerializeAll(std::vector<SerializedEntity> &out) const;
  size_t EntityCount() const;

 private:
  std::vector<std::unique_ptr<GameEntity>> entities_;
};

#endif
