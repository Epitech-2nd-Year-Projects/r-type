#ifndef ENGINE_DATA_STRUCTURES_SPATIAL_GRID_H_
#define ENGINE_DATA_STRUCTURES_SPATIAL_GRID_H_

#include <cmath>
#include <cstdint>
#include <functional>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "engine/math/rect.h"

namespace engine::data_structures {

/**
 * @class SpatialGrid
 * @brief Generic spatial partitioning grid for optimizing 2D queries.
 * @tparam EntityType The type of entity ID to store (e.g., uint32_t, EntityId).
 *
 * @details
 * Divides 2D space into a grid of fixed-size cells. Objects are inserted
 * into all cells they overlap. This transforms O(N^2) collision checks
 * into ~O(N) for typical game scenarios.
 */
template <typename EntityType>
class SpatialGrid {
 public:
  /**
   * @brief Construct a new Spatial Grid
   * @param cell_size Dimensions of each grid cell (width/height).
   */
  explicit SpatialGrid(float cell_size = 100.0f) : cell_size_(cell_size) {}

  /**
   * @brief Clear all entries from the grid.
   */
  void Clear() { grid_.clear(); }

  /**
   * @brief Insert an entity with its bounding box into the grid.
   * @param entity The entity identifier.
   * @param bounds The 2D bounding box of the entity in world space.
   */
  void Insert(EntityType entity, const engine::math::RectF& bounds) {
    int min_x = static_cast<int>(std::floor(bounds.top_left_x_ / cell_size_));
    int max_x = static_cast<int>(
        std::floor((bounds.top_left_x_ + bounds.width_) / cell_size_));
    int min_y = static_cast<int>(std::floor(bounds.top_left_y_ / cell_size_));
    int max_y = static_cast<int>(
        std::floor((bounds.top_left_y_ + bounds.height_) / cell_size_));

    for (int x = min_x; x <= max_x; ++x) {
      for (int y = min_y; y <= max_y; ++y) {
        grid_[{x, y}].push_back(entity);
      }
    }
  }

  /**
   * @brief Callback function signature for collision pairs.
   */
  using CollisionCallback = std::function<void(EntityType, EntityType)>;

  /**
   * @brief Iterate over all potential collision pairs in the grid.
   *
   * @details
   * Iterates through each cell and checks all unique pairs within that cell.
   * A pair (A, B) is only reported once, even if they share multiple cells.
   * The callback is invoked for every unique pair found in the same cell.
   * The actual geometric intersection check should be done inside the callback.
   *
   * @param callback Function to call for each potential pair.
   */
  void ForEachPotentialCollision(CollisionCallback callback) {
    std::unordered_set<std::uint64_t> checked_pairs;

    for (const auto& [key, entities] : grid_) {
      if (entities.size() < 2) continue;

      for (size_t i = 0; i < entities.size(); ++i) {
        for (size_t j = i + 1; j < entities.size(); ++j) {
          EntityType e1 = entities[i];
          EntityType e2 = entities[j];

          if (e1 > e2) std::swap(e1, e2);

          std::uint64_t pair_hash = PairHash(e1, e2);
          if (checked_pairs.count(pair_hash)) continue;

          checked_pairs.insert(pair_hash);
          callback(e1, e2);
        }
      }
    }
  }

 private:
  struct GridKey {
    int x;
    int y;

    bool operator==(const GridKey& other) const {
      return x == other.x && y == other.y;
    }
  };

  struct GridKeyHash {
    std::size_t operator()(const GridKey& k) const {
      return std::hash<int>()(k.x) ^
             (std::hash<int>()(k.y) + 0x9e3779b9 +
              (std::hash<int>()(k.x) << 6) + (std::hash<int>()(k.x) >> 2));
    }
  };

  /**
   * @brief Hashes a pair of entity IDs to track checked pairs.
   * Assumes entity ID fits in 32 bits for the 64-bit combined hash.
   */
  std::uint64_t PairHash(EntityType a, EntityType b) const {
    return (static_cast<std::uint64_t>(a) << 32) |
           static_cast<std::uint64_t>(b);
  }

  float cell_size_;
  std::unordered_map<GridKey, std::vector<EntityType>, GridKeyHash> grid_;
};

}  // namespace engine::data_structures

#endif  // ENGINE_DATA_STRUCTURES_SPATIAL_GRID_H_
