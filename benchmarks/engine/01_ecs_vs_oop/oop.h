#ifndef BENCH_OOP_HPP
#define BENCH_OOP_HPP

#include <memory>
#include <vector>

class GameEntity {
public:
  virtual ~GameEntity() = default;
  virtual void UpdatePhysics(float dt) = 0;
  virtual void UpdateHealth(float dt) = 0;

protected:
  float x_ = 0.0f;
  float y_ = 0.0f;
};

class Player : public GameEntity {
private:
  float vx_ = 1.0f;
  float vy_ = 1.5f;
  int health_ = 100;

public:
  void UpdatePhysics(float dt) override;
  void UpdateHealth(float dt) override;
};

class Enemy : public GameEntity {
private:
  float vx_ = -1.0f;
  float vy_ = 0.5f;
  int health_ = 50;

public:
  void UpdatePhysics(float dt) override;
  void UpdateHealth(float dt) override;
};

class OOPGameWorld {
public:
  void AddEntity(std::unique_ptr<GameEntity> entity);
  void UpdatePhysics(float dt);
  void UpdateHealth(float dt);
  size_t EntityCount() const;

private:
  std::vector<std::unique_ptr<GameEntity>> entities_;
};

#endif