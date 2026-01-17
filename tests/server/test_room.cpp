#include <gtest/gtest.h>

#include "engine/util/logging.h"
#include "protocol/lobby.h"
#include "room.h"

TEST(RoomTest, DifficultyStorage) {
  auto& logger = engine::util::Logger::Default();
  server::Room room("test_code", "test_name", 1, 4, false, "", 1234,
                    protocol::Difficulty::kHard, logger);
  EXPECT_EQ(room.Difficulty(), protocol::Difficulty::kHard);
}
