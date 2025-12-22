#include "engine/data_structures/spatial_grid.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <utility>
#include <vector>

#include "engine/math/rect.h"

using namespace engine::data_structures;
using namespace engine::math;

TEST(SpatialGridTest, InsertAndRetrieveCollision) {
  SpatialGrid<int> grid(100.0f);

  grid.Insert(1, RectF(0.0f, 0.0f, 50.0f, 50.0f));
  grid.Insert(2, RectF(10.0f, 10.0f, 50.0f, 50.0f));
  grid.Insert(3, RectF(200.0f, 200.0f, 50.0f, 50.0f));

  std::vector<std::pair<int, int>> collisions;
  grid.ForEachPotentialCollision([&](int a, int b) {
    if (a > b) std::swap(a, b);
    collisions.push_back({a, b});
  });

  ASSERT_EQ(collisions.size(), 1);
  EXPECT_EQ(collisions[0].first, 1);
  EXPECT_EQ(collisions[0].second, 2);
}

TEST(SpatialGridTest, MultiCellOccupation) {
  SpatialGrid<int> grid(10.0f);

  grid.Insert(1, RectF(0.0f, 0.0f, 20.0f, 20.0f));
  grid.Insert(2, RectF(5.0f, 5.0f, 5.0f, 5.0f));

  std::vector<std::pair<int, int>> collisions;
  grid.ForEachPotentialCollision([&](int a, int b) {
    if (a > b) std::swap(a, b);
    collisions.push_back({a, b});
  });

  ASSERT_EQ(collisions.size(), 1);
  EXPECT_EQ(collisions[0].first, 1);
  EXPECT_EQ(collisions[0].second, 2);
}

TEST(SpatialGridTest, Clear) {
  SpatialGrid<int> grid(100.0f);
  grid.Insert(1, RectF(0, 0, 10, 10));
  grid.Insert(2, RectF(0, 0, 10, 10));

  grid.Clear();

  int count = 0;
  grid.ForEachPotentialCollision([&](int, int) { count++; });
  EXPECT_EQ(count, 0);
}
