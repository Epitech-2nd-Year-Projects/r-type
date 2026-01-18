#include "world_builder.h"

#include "engine/render/renderer2d.h"
#include "engine/render/renderer3d.h"

namespace rift::client {

namespace {

const char* kBuildingCastlePath =
    "rift/client/assets/world/buildings/building_castle_blue.gltf";
const char* kBuildingTowerAPath =
    "rift/client/assets/world/buildings/building_tower_A_blue.gltf";
const char* kBuildingTowerBPath =
    "rift/client/assets/world/buildings/building_tower_B_blue.gltf";
const char* kBuildingHomeAPath =
    "rift/client/assets/world/buildings/building_home_A_blue.gltf";
const char* kBuildingHomeBPath =
    "rift/client/assets/world/buildings/building_home_B_blue.gltf";
const char* kBuildingTavernPath =
    "rift/client/assets/world/buildings/building_tavern_blue.gltf";
const char* kBuildingChurchPath =
    "rift/client/assets/world/buildings/building_church_blue.gltf";
const char* kBuildingBlacksmithPath =
    "rift/client/assets/world/buildings/building_blacksmith_blue.gltf";
const char* kBuildingWellPath =
    "rift/client/assets/world/buildings/building_well_blue.gltf";

constexpr float kBuildingScale = 5.0f;
constexpr float kBuildingRowZ = -12.0f;

}  // namespace

WorldBuilder::WorldBuilder(engine::render::Renderer3D& renderer)
    : renderer_(renderer) {}

void WorldBuilder::Initialize(float arena_width, float arena_depth) {
  if (initialized_) return;

  LoadModels();
  GenerateHexGround(arena_width, arena_depth);
  PlaceBuildings(arena_width, arena_depth);

  initialized_ = true;
}

void WorldBuilder::LoadModels() {
  building_castle_ = renderer_.LoadModelFromFile(kBuildingCastlePath);
  building_tower_a_ = renderer_.LoadModelFromFile(kBuildingTowerAPath);
  building_tower_b_ = renderer_.LoadModelFromFile(kBuildingTowerBPath);
  building_home_a_ = renderer_.LoadModelFromFile(kBuildingHomeAPath);
  building_home_b_ = renderer_.LoadModelFromFile(kBuildingHomeBPath);
  building_tavern_ = renderer_.LoadModelFromFile(kBuildingTavernPath);
  building_church_ = renderer_.LoadModelFromFile(kBuildingChurchPath);
  building_blacksmith_ = renderer_.LoadModelFromFile(kBuildingBlacksmithPath);
  building_well_ = renderer_.LoadModelFromFile(kBuildingWellPath);
}

void WorldBuilder::GenerateHexGround(float arena_width, float arena_depth) {
  (void)arena_width;
  (void)arena_depth;
  ground_width_ = 500.0f;
  ground_depth_ = 500.0f;
  ground_center_z_ = -200.0f;
}

void WorldBuilder::PlaceBuildings(float arena_width, float arena_depth) {
  (void)arena_depth;

  const float half_width = arena_width / 2.0f + 4.0f;

  if (building_castle_) {
    WorldObject castle;
    castle.model = building_castle_;
    castle.position = {0.0f, 0.0f, kBuildingRowZ - 4.0f};
    castle.rotation_y = 0.0f;
    castle.scale = kBuildingScale;
    buildings_.push_back(castle);
  }

  if (building_tower_a_) {
    WorldObject tower_left;
    tower_left.model = building_tower_a_;
    tower_left.position = {-10.0f, 0.0f, kBuildingRowZ - 2.0f};
    tower_left.rotation_y = 15.0f;
    tower_left.scale = kBuildingScale;
    buildings_.push_back(tower_left);
  }

  if (building_tower_b_) {
    WorldObject tower_right;
    tower_right.model = building_tower_b_;
    tower_right.position = {10.0f, 0.0f, kBuildingRowZ - 2.0f};
    tower_right.rotation_y = -15.0f;
    tower_right.scale = kBuildingScale;
    buildings_.push_back(tower_right);
  }

  if (building_home_a_) {
    WorldObject home1;
    home1.model = building_home_a_;
    home1.position = {-half_width, 0.0f, kBuildingRowZ + 2.0f};
    home1.rotation_y = 30.0f;
    home1.scale = kBuildingScale;
    buildings_.push_back(home1);
  }

  if (building_home_b_) {
    WorldObject home2;
    home2.model = building_home_b_;
    home2.position = {-half_width + 8.0f, 0.0f, kBuildingRowZ};
    home2.rotation_y = -10.0f;
    home2.scale = kBuildingScale;
    buildings_.push_back(home2);
  }

  if (building_tavern_) {
    WorldObject tavern;
    tavern.model = building_tavern_;
    tavern.position = {half_width, 0.0f, kBuildingRowZ + 2.0f};
    tavern.rotation_y = -25.0f;
    tavern.scale = kBuildingScale;
    buildings_.push_back(tavern);
  }

  if (building_church_) {
    WorldObject church;
    church.model = building_church_;
    church.position = {half_width - 8.0f, 0.0f, kBuildingRowZ};
    church.rotation_y = 10.0f;
    church.scale = kBuildingScale;
    buildings_.push_back(church);
  }

  if (building_blacksmith_) {
    WorldObject blacksmith;
    blacksmith.model = building_blacksmith_;
    blacksmith.position = {-5.0f, 0.0f, kBuildingRowZ + 4.0f};
    blacksmith.rotation_y = 5.0f;
    blacksmith.scale = kBuildingScale;
    buildings_.push_back(blacksmith);
  }

  if (building_well_) {
    WorldObject well;
    well.model = building_well_;
    well.position = {5.0f, 0.0f, kBuildingRowZ + 4.0f};
    well.rotation_y = 0.0f;
    well.scale = kBuildingScale;
    buildings_.push_back(well);
  }
}

void WorldBuilder::Draw(engine::render::Renderer3D& renderer) {
  if (!initialized_) return;

  renderer.DrawPlane({0.0f, 0.0f, ground_center_z_},
                     {ground_width_, ground_depth_},
                     engine::render::Color::FromBytes(75, 115, 55));

  engine::render::ModelDrawParams params;
  for (const auto& building : buildings_) {
    if (!building.model) continue;

    params.position = building.position;
    params.rotation_axis = {0.0f, 1.0f, 0.0f};
    params.rotation_angle = building.rotation_y;
    params.scale = {building.scale, building.scale, building.scale};
    params.tint = engine::render::Color::White();

    renderer.DrawModel(*building.model, params);
  }
}

void WorldBuilder::DrawSky(engine::render::Renderer2D& renderer,
                           int screen_width, int screen_height) {
  constexpr int kNumStrips = 32;
  const float strip_height =
      static_cast<float>(screen_height) / static_cast<float>(kNumStrips);

  constexpr int kTopR = 100, kTopG = 160, kTopB = 220;
  constexpr int kBottomR = 180, kBottomG = 210, kBottomB = 240;

  for (int i = 0; i < kNumStrips; ++i) {
    const float t = static_cast<float>(i) / static_cast<float>(kNumStrips - 1);

    const int r = kTopR + static_cast<int>(t * (kBottomR - kTopR));
    const int g = kTopG + static_cast<int>(t * (kBottomG - kTopG));
    const int b = kTopB + static_cast<int>(t * (kBottomB - kTopB));

    const float y = static_cast<float>(i) * strip_height;

    renderer.DrawRect({0.0f, y, static_cast<float>(screen_width),
                       strip_height + 1.0f},  // +1 to avoid gaps
                      engine::render::Color::FromBytes(r, g, b));
  }
}

}  // namespace rift::client
