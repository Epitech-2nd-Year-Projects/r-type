#ifndef RIFT_CLIENT_WORLD_BUILDER_H_
#define RIFT_CLIENT_WORLD_BUILDER_H_

#include <memory>
#include <vector>

#include "engine/math/vector3.h"
#include "engine/render/model.h"

namespace engine::render {
class Renderer2D;
class Renderer3D;
}

namespace rift::client {

struct WorldObject {
  std::shared_ptr<engine::render::Model> model;
  engine::math::Vector3f position;
  float rotation_y{0.0f};
  float scale{1.0f};
};

class WorldBuilder {
 public:
  explicit WorldBuilder(engine::render::Renderer3D& renderer);

  /// Load all world assets and generate the layout.
  /// @param arena_width The width of the arena in 3D units.
  /// @param arena_depth The depth of the arena in 3D units.
  void Initialize(float arena_width, float arena_depth);

  /// Draw sky background (call before Begin3D).
  /// @param renderer The 2D renderer for drawing sky gradient.
  /// @param screen_width Screen width in pixels.
  /// @param screen_height Screen height in pixels.
  void DrawSky(engine::render::Renderer2D& renderer, int screen_width,
               int screen_height);

  /// Draw all world objects (call within Begin3D/End3D).
  void Draw(engine::render::Renderer3D& renderer);

  /// Check if the world has been initialized.
  [[nodiscard]] bool IsInitialized() const { return initialized_; }

 private:
  void LoadModels();
  void GenerateHexGround(float arena_width, float arena_depth);
  void PlaceBuildings(float arena_width, float arena_depth);

  engine::render::Renderer3D& renderer_;
  bool initialized_{false};

  std::shared_ptr<engine::render::Model> building_castle_;
  std::shared_ptr<engine::render::Model> building_tower_a_;
  std::shared_ptr<engine::render::Model> building_tower_b_;
  std::shared_ptr<engine::render::Model> building_home_a_;
  std::shared_ptr<engine::render::Model> building_home_b_;
  std::shared_ptr<engine::render::Model> building_tavern_;
  std::shared_ptr<engine::render::Model> building_church_;
  std::shared_ptr<engine::render::Model> building_blacksmith_;
  std::shared_ptr<engine::render::Model> building_well_;

  float ground_width_{0.0f};
  float ground_depth_{0.0f};
  float ground_center_z_{0.0f};

  std::vector<WorldObject> buildings_;
};

}  // namespace rift::client

#endif  // RIFT_CLIENT_WORLD_BUILDER_H_
