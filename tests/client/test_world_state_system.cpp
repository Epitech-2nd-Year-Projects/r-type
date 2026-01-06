#include <gtest/gtest.h>

#include "ecs/world_state_system.h"
#include "engine/ecs/registry.h"
#include "engine/ecs/sparse_array.h"
#include "protocol/world_snapshot.h"

namespace {

using client::ecs::HealthComponent;
using client::ecs::NetworkedEntityComponent;
using client::ecs::PositionComponent;
using client::ecs::VelocityComponent;

std::size_t FindEntityIndex(
    const engine::ecs::SparseArray<NetworkedEntityComponent>& net,
    std::uint32_t network_id) {
  for (std::size_t i = 0; i < net.size(); ++i) {
    if (net[i].has_value() && net[i]->network_id == network_id) {
      return i;
    }
  }
  return net.size();
}

protocol::EntityDelta MakeCreateDelta(std::uint32_t id, std::uint16_t type,
                                      std::int16_t x, std::int16_t y,
                                      std::int16_t vx, std::int16_t vy,
                                      std::uint8_t hp) {
  protocol::EntityDelta delta{};
  delta.op = protocol::EntityDeltaOp::kCreate;
  delta.entity_id = id;
  delta.state.entity_id = id;
  delta.state.type = type;
  delta.state.x = x;
  delta.state.y = y;
  delta.state.vx = vx;
  delta.state.vy = vy;
  delta.state.hp = hp;
  return delta;
}

void ApplyAt(client::ecs::WorldStateSystem& system,
             const protocol::WorldSnapshotPayload& snapshot,
             std::uint64_t timestamp_ms) {
  system.ApplySnapshot(snapshot, timestamp_ms);
}

}  // namespace

TEST(WorldStateSystemTest, AppliesCreateSnapshot) {
  engine::ecs::Registry registry;
  client::ecs::WorldStateSystem system(registry);

  protocol::WorldSnapshotPayload snapshot{};
  snapshot.snapshot_id = 1;
  snapshot.base_snapshot_id = protocol::kNoBaseSnapshotId;
  snapshot.deltas.push_back(MakeCreateDelta(5, 2, 10, -4, 3, -1, 7));

  ApplyAt(system, snapshot, 10);

  const auto& net = registry.GetComponents<NetworkedEntityComponent>();
  const auto index = FindEntityIndex(net, 5);
  ASSERT_LT(index, net.size());
  const auto& pos = registry.GetComponents<PositionComponent>()[index];
  const auto& vel = registry.GetComponents<VelocityComponent>()[index];
  const auto& hp = registry.GetComponents<HealthComponent>()[index];

  EXPECT_EQ(net[index]->network_id, 5u);
  EXPECT_EQ(net[index]->type_code, 2u);
  EXPECT_EQ(net[index]->last_snapshot, 1u);
  ASSERT_TRUE(pos.has_value());
  EXPECT_FLOAT_EQ(pos->position.x, 10.0f);
  EXPECT_FLOAT_EQ(pos->position.y, -4.0f);
  ASSERT_TRUE(vel.has_value());
  EXPECT_FLOAT_EQ(vel->velocity.x, 3.0f);
  EXPECT_FLOAT_EQ(vel->velocity.y, -1.0f);
  ASSERT_TRUE(hp.has_value());
  EXPECT_EQ(hp->current, 7u);
  EXPECT_EQ(hp->max, 7u);
}

TEST(WorldStateSystemTest, TracksPlayerStatsForPlayers) {
  engine::ecs::Registry registry;
  client::ecs::WorldStateSystem system(registry);

  protocol::WorldSnapshotPayload snapshot{};
  snapshot.snapshot_id = 1;
  snapshot.base_snapshot_id = protocol::kNoBaseSnapshotId;

  protocol::EntityDelta delta{};
  delta.op = protocol::EntityDeltaOp::kCreate;
  delta.entity_id = 3;
  delta.state.entity_id = 3;
  delta.state.type = 1;
  delta.state.x = 0;
  delta.state.y = 0;
  delta.state.vx = 0;
  delta.state.vy = 0;
  delta.state.hp = 5;
  delta.state.player_id = 42;
  delta.state.score = 250;
  delta.state.lives = 2;
  snapshot.deltas.push_back(delta);

  ApplyAt(system, snapshot, 10);

  const auto& net = registry.GetComponents<NetworkedEntityComponent>();
  const auto index = FindEntityIndex(net, 3);
  ASSERT_LT(index, net.size());
  const auto& player_states =
      registry.GetComponents<client::ecs::PlayerStateComponent>()[index];
  ASSERT_TRUE(player_states.has_value());
  EXPECT_EQ(player_states->player_id, 42u);
  EXPECT_EQ(player_states->score, 250u);
  EXPECT_EQ(player_states->lives, 2u);
}

TEST(WorldStateSystemTest, AppliesUpdateAndTracksPreviousPosition) {
  engine::ecs::Registry registry;
  client::ecs::WorldStateSystem system(registry);

  protocol::WorldSnapshotPayload create{};
  create.snapshot_id = 1;
  create.base_snapshot_id = protocol::kNoBaseSnapshotId;
  create.deltas.push_back(MakeCreateDelta(1, 1, 0, 0, 0, 0, 5));
  ApplyAt(system, create, 10);

  protocol::WorldSnapshotPayload update{};
  update.snapshot_id = 2;
  update.base_snapshot_id = 1;
  protocol::EntityDelta delta{};
  delta.op = protocol::EntityDeltaOp::kUpdate;
  delta.entity_id = 1;
  delta.field_mask =
      protocol::EntityFieldMask::kFieldX | protocol::EntityFieldMask::kFieldHp;
  delta.state.x = 8;
  delta.state.hp = 3;
  update.deltas.push_back(delta);
  ApplyAt(system, update, 20);

  const auto index =
      FindEntityIndex(registry.GetComponents<NetworkedEntityComponent>(), 1);
  ASSERT_LT(index, registry.GetComponents<NetworkedEntityComponent>().size());
  const auto& pos = registry.GetComponents<PositionComponent>()[index];
  const auto& hp = registry.GetComponents<HealthComponent>()[index];
  ASSERT_TRUE(pos.has_value());
  EXPECT_FLOAT_EQ(pos->previous_position.x, 0.0f);
  EXPECT_FLOAT_EQ(pos->position.x, 8.0f);
  ASSERT_TRUE(hp.has_value());
  EXPECT_EQ(hp->current, 3u);
  EXPECT_EQ(hp->max, 5u);
}

TEST(WorldStateSystemTest, DeletesEntitiesOnDelta) {
  engine::ecs::Registry registry;
  client::ecs::WorldStateSystem system(registry);

  protocol::WorldSnapshotPayload snapshot{};
  snapshot.snapshot_id = 1;
  snapshot.base_snapshot_id = protocol::kNoBaseSnapshotId;
  snapshot.deltas.push_back(MakeCreateDelta(2, 1, 0, 0, 0, 0, 1));
  ApplyAt(system, snapshot, 10);

  protocol::WorldSnapshotPayload deletion{};
  deletion.snapshot_id = 2;
  deletion.base_snapshot_id = 1;
  protocol::EntityDelta delta{};
  delta.op = protocol::EntityDeltaOp::kDelete;
  delta.entity_id = 2;
  deletion.deltas.push_back(delta);
  ApplyAt(system, deletion, 20);

  const auto& net = registry.GetComponents<NetworkedEntityComponent>();
  EXPECT_EQ(FindEntityIndex(net, 2), net.size());
}

TEST(WorldStateSystemTest, UpdateCreatesMissingEntityWithMaskedFields) {
  engine::ecs::Registry registry;
  client::ecs::WorldStateSystem system(registry);

  protocol::WorldSnapshotPayload update{};
  update.snapshot_id = 1;
  update.base_snapshot_id = protocol::kNoBaseSnapshotId;
  protocol::EntityDelta delta{};
  delta.op = protocol::EntityDeltaOp::kUpdate;
  delta.entity_id = 9;
  delta.field_mask =
      protocol::EntityFieldMask::kFieldX | protocol::EntityFieldMask::kFieldHp;
  delta.state.x = 12;
  delta.state.hp = 6;
  update.deltas.push_back(delta);
  ApplyAt(system, update, 10);

  const auto& net = registry.GetComponents<NetworkedEntityComponent>();
  const auto index = FindEntityIndex(net, 9);
  ASSERT_LT(index, net.size());
  const auto& pos = registry.GetComponents<PositionComponent>()[index];
  const auto& vel = registry.GetComponents<VelocityComponent>()[index];
  const auto& hp = registry.GetComponents<HealthComponent>()[index];

  ASSERT_TRUE(pos.has_value());
  EXPECT_FLOAT_EQ(pos->position.x, 12.0f);
  EXPECT_FLOAT_EQ(pos->previous_position.x, 12.0f);
  EXPECT_FALSE(vel.has_value());
  ASSERT_TRUE(hp.has_value());
  EXPECT_EQ(hp->current, 6u);
  EXPECT_EQ(hp->max, 6u);
}

TEST(WorldStateSystemTest, FullSnapshotPrunesMissingEntities) {
  engine::ecs::Registry registry;
  client::ecs::WorldStateSystem system(registry);

  protocol::WorldSnapshotPayload initial{};
  initial.snapshot_id = 1;
  initial.base_snapshot_id = protocol::kNoBaseSnapshotId;
  initial.deltas.push_back(MakeCreateDelta(10, 1, 1, 1, 0, 0, 5));
  initial.deltas.push_back(MakeCreateDelta(11, 1, 2, 2, 0, 0, 5));
  ApplyAt(system, initial, 10);

  protocol::WorldSnapshotPayload resync{};
  resync.snapshot_id = 2;
  resync.base_snapshot_id = protocol::kNoBaseSnapshotId;
  resync.deltas.push_back(MakeCreateDelta(11, 1, 5, 5, 0, 0, 4));
  ApplyAt(system, resync, 20);

  const auto& net = registry.GetComponents<NetworkedEntityComponent>();
  const auto index_missing = FindEntityIndex(net, 10);
  const auto index_kept = FindEntityIndex(net, 11);
  EXPECT_EQ(index_missing, net.size());
  ASSERT_LT(index_kept, net.size());
  EXPECT_EQ(net[index_kept]->last_snapshot, 2u);
}

TEST(WorldStateSystemTest, RejectsStaleSnapshots) {
  engine::ecs::Registry registry;
  client::ecs::WorldStateSystem system(registry);

  protocol::WorldSnapshotPayload first{};
  first.snapshot_id = 2;
  first.base_snapshot_id = protocol::kNoBaseSnapshotId;
  first.deltas.push_back(MakeCreateDelta(4, 1, 1, 1, 0, 0, 9));
  ApplyAt(system, first, 20);

  protocol::WorldSnapshotPayload stale{};
  stale.snapshot_id = 1;
  stale.base_snapshot_id = protocol::kNoBaseSnapshotId;
  stale.deltas.push_back(MakeCreateDelta(4, 1, 5, 5, 0, 0, 1));
  ApplyAt(system, stale, 10);

  const auto& net = registry.GetComponents<NetworkedEntityComponent>();
  const auto index = FindEntityIndex(net, 4);
  ASSERT_LT(index, net.size());
  const auto& pos = registry.GetComponents<PositionComponent>()[index];
  const auto& hp = registry.GetComponents<HealthComponent>()[index];
  ASSERT_TRUE(pos.has_value());
  EXPECT_FLOAT_EQ(pos->position.x, 1.0f);
  ASSERT_TRUE(hp.has_value());
  EXPECT_EQ(hp->current, 9u);
  EXPECT_EQ(system.last_snapshot_id(), 2u);
}
