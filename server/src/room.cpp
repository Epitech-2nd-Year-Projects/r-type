#include "room.h"

#include <optional>
#include <utility>

namespace server {

Room::Room(std::string room_code, std::string room_name, std::uint32_t room_id,
           std::uint16_t max_players, bool is_private, std::string password,
           std::uint32_t seed, engine::util::Logger& logger)
    : room_code_(std::move(room_code)),
      room_name_(std::move(room_name)),
      room_id_(room_id),
      max_players_(max_players),
      is_private_(is_private),
      password_(std::move(password)),
      seed_(seed),
      started_(false),
      game_instance_(
          std::make_unique<GameInstance>(room_id, seed, max_players, logger)),
      snapshot_history_(32),
      logger_(logger) {}

Room::Room(Room&& other) noexcept
    : room_code_(std::move(other.room_code_)),
      room_name_(std::move(other.room_name_)),
      room_id_(other.room_id_),
      max_players_(other.max_players_),
      is_private_(other.is_private_),
      password_(std::move(other.password_)),
      seed_(other.seed_),
      next_snapshot_id_(other.next_snapshot_id_),
      room_tick_(other.room_tick_),
      last_active_ms_(other.last_active_ms_),
      started_(other.started_),
      players_(std::move(other.players_)),
      game_instance_(std::move(other.game_instance_)),
      snapshot_history_(std::move(other.snapshot_history_)),
      logger_(other.logger_) {}

void Room::Update(const engine::time::TimeDelta& delta) {
  if (game_instance_) {
    game_instance_->Update(delta);
    if (game_instance_->Logic().IsRunning()) {
      started_ = true;
    }
  }
  ++room_tick_;
}

bool Room::AddPlayer(std::uint32_t player_id, std::string_view player_name) {
  if (!game_instance_) {
    return false;
  }
  if (players_.size() >= max_players_) {
    return false;
  }
  const auto [_, inserted] = players_.insert(player_id);
  if (!inserted) {
    return false;
  }
  game_instance_->OnPlayerJoined(player_id, player_name);
  return true;
}

bool Room::RemovePlayer(std::uint32_t player_id) {
  if (!game_instance_) {
    return false;
  }
  const auto it = players_.find(player_id);
  if (it == players_.end()) {
    return false;
  }
  game_instance_->OnPlayerLeft(player_id);
  players_.erase(it);
  return true;
}

void Room::HandleInput(std::uint32_t player_id,
                       const protocol::InputStatePayload& payload,
                       const protocol::Header& header) {
  if (game_instance_) {
    game_instance_->OnPlayerInput(player_id, payload, header);
  }
}

std::optional<GameInstance::ReadyEvent> Room::HandleClientCommand(
    std::uint32_t player_id, const protocol::CommandPayload& command) {
  if (!game_instance_) {
    return std::nullopt;
  }
  return game_instance_->OnClientCommand(player_id, command);
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

void Room::MarkActive(std::uint32_t timestamp_ms) {
  last_active_ms_ = timestamp_ms;
}

std::vector<protocol::PlayerDiedPayload> Room::PollPlayerDeaths() {
  std::vector<protocol::PlayerDiedPayload> payloads;
  if (!game_instance_) {
    return payloads;
  }
  auto events = game_instance_->Logic().ExtractPlayerDeathEvents();
  payloads.reserve(events.size());
  for (const auto& evt : events) {
    protocol::PlayerDiedPayload payload;
    payload.player_id = evt.player_id;
    payload.remaining_lives = evt.remaining_lives;
    payload.killer_entity_id = 0;
    payload.cause = protocol::DeathCause::kUnknown;
    payloads.push_back(payload);
  }
  return payloads;
}

std::optional<std::reference_wrapper<const protocol::WorldSnapshotPayload>> Room::GetSnapshot(std::uint32_t snapshot_id) const {
  return snapshot_history_.GetSnapshot(snapshot_id);
}

const std::string& Room::Code() const { return room_code_; }
const std::string& Room::Name() const { return room_name_; }

std::uint32_t Room::Id() const { return room_id_; }

std::uint16_t Room::MaxPlayers() const { return max_players_; }

bool Room::IsPrivate() const { return is_private_; }
const std::string& Room::Password() const { return password_; }

std::uint32_t Room::Seed() const { return seed_; }

std::size_t Room::PlayerCount() const { return players_.size(); }

const std::unordered_set<std::uint32_t>& Room::Players() const {
  return players_;
}

std::uint32_t Room::LastActiveMs() const { return last_active_ms_; }

bool Room::IsEmpty() const { return players_.empty(); }

bool Room::HasStarted() const { return started_; }

}  // namespace server
