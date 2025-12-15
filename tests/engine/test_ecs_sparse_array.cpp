#include <gtest/gtest.h>

#include <optional>
#include <vector>

#include "engine/ecs/sparse_array.h"

struct Position {
  float x, y;

  bool operator==(const Position& other) const {
    return x == other.x && y == other.y;
  }
};

TEST(SparseArrayTest, InsertAndAccess) {
  engine::ecs::SparseArray<Position> positions;

  positions.EmplaceAt(0, 10.0f, 20.0f);
  ASSERT_EQ(positions.size(), 1);
  ASSERT_TRUE(positions[0].has_value());
  EXPECT_EQ(positions[0]->x, 10.0f);
  EXPECT_EQ(positions[0]->y, 20.0f);

  positions.EmplaceAt(2, 30.0f, 40.0f);
  ASSERT_EQ(positions.size(), 3);
  ASSERT_FALSE(positions[1].has_value());
  ASSERT_TRUE(positions[2].has_value());
  EXPECT_EQ(positions[2]->x, 30.0f);
}

TEST(SparseArrayTest, Erase) {
  engine::ecs::SparseArray<int> ints;

  ints.InsertAt(0, 42);
  ASSERT_TRUE(ints[0].has_value());

  ints.Erase(0);
  ASSERT_FALSE(ints[0].has_value());

  ints.Erase(5);
  ints.Erase(0);
}

TEST(SparseArrayTest, Iterators) {
  engine::ecs::SparseArray<int> ints;

  ints.InsertAt(0, 10);
  ints.InsertAt(2, 20);

  int count_active = 0;
  for (const auto& val : ints) {
    if (val.has_value()) {
      count_active++;
    }
  }
  EXPECT_EQ(count_active, 2);
}

TEST(SparseArrayTest, OutOfBoundsAccess) {
  engine::ecs::SparseArray<int> ints;

  const auto& const_ints = ints;
  EXPECT_FALSE(const_ints[100].has_value());

  ints[5] = 42;
  EXPECT_EQ(ints.size(), 6);
  ASSERT_TRUE(ints[5].has_value());
  EXPECT_EQ(ints[5].value(), 42);
}
