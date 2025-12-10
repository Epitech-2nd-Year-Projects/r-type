#include "room.h"

#include <utility>

namespace server {

Room::Room(std::string room_code,
           std::uint32_t room_id,
           std::uint16_t max_players,
           std::uint32_t seed)
    : room_code_(std::move(room_code)),
      room_id_(room_id),
      max_players_(max_players),
      seed_(seed),
      game_instance_(std::make_unique<GameInstance>(room_id, seed, max_players)),
      snapshot_history_(32) {}

void Room::Update(const engine::time::TimeDelta& delta) {
  if (game_instance_) {
    game_instance_->Update(delta);
  }
}

void Room::AddPlayer(std::uint32_t player_id, std::string_view player_name) {
  if (game_instance_ && players_.insert(player_id).second) {
    game_instance_->OnPlayerJoined(player_id, player_name);
  }
}

void Room::RemovePlayer(std::uint32_t player_id) {
  if (!game_instance_) {
    return;
  }
  const auto it = players_.find(player_id);
  if (it == players_.end()) {
    return;
  }
  game_instance_->OnPlayerLeft(player_id);
  players_.erase(it);
}

void Room::HandleInput(std::uint32_t player_id,
                       const protocol::InputStatePayload& payload,
                       const protocol::Header& header) {
  if (game_instance_) {
    game_instance_->OnPlayerInput(player_id, payload, header);
  }
}

protocol::WorldSnapshotPayload Room::BuildSnapshot(std::uint32_t server_tick) {
  if (!game_instance_) {
    return {};
  }
  protocol::WorldSnapshotPayload snapshot =
      game_instance_->BuildWorldSnapshot(next_snapshot_id_++, server_tick);
  snapshot_history_.AddSnapshot(snapshot);
  return snapshot;
}

void Room::MarkActive(std::uint32_t timestamp_ms) { last_active_ms_ = timestamp_ms; }

const std::string& Room::Code() const { return room_code_; }

std::uint32_t Room::Id() const { return room_id_; }

std::uint16_t Room::MaxPlayers() const { return max_players_; }

std::uint32_t Room::Seed() const { return seed_; }

std::size_t Room::PlayerCount() const { return players_.size(); }

std::uint32_t Room::LastActiveMs() const { return last_active_ms_; }

bool Room::IsEmpty() const { return players_.empty(); }

}  // namespace server
