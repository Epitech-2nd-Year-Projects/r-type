#include "lag_compensation.h"

#include <algorithm>

namespace server {

LagCompensationHistory::LagCompensationHistory(
    std::uint32_t history_duration_ms)
    : max_history_duration_ms_(history_duration_ms) {}

void LagCompensationHistory::RecordSnapshot(std::uint32_t server_tick,
                                            std::uint32_t timestamp_ms,
                                            std::uint32_t player_id,
                                            engine::math::Vector2f position) {
  PlayerSnapshot snapshot{server_tick, timestamp_ms, player_id, position};
  auto& ph = history_[player_id];

  if (!ph.snapshots.empty()) {
    std::uint32_t last_ts = ph.snapshots.back().timestamp_ms;
    std::int32_t diff = static_cast<std::int32_t>(timestamp_ms - last_ts);
    if (diff <= 0) {
      return;
    }
  }

  ph.snapshots.push_back(snapshot);
  PruneOldSnapshots(ph, timestamp_ms);
}

void LagCompensationHistory::PruneOldSnapshots(PlayerHistory& ph,
                                               std::uint32_t current_time_ms) {
  while (ph.snapshots.size() > 1) {
    std::uint32_t front_ts = ph.snapshots.front().timestamp_ms;
    std::uint32_t age = current_time_ms - front_ts;
    if (age > max_history_duration_ms_) {
      ph.snapshots.pop_front();
    } else {
      break;
    }
  }
}

std::optional<engine::math::Vector2f>
LagCompensationHistory::GetPlayerPositionAt(
    std::uint32_t player_id, std::uint32_t target_time_ms) const {
  auto it = history_.find(player_id);
  if (it == history_.end() || it->second.snapshots.empty()) {
    return std::nullopt;
  }

  const auto& snapshots = it->second.snapshots;

  if (target_time_ms <= snapshots.front().timestamp_ms) {
    return snapshots.front().position;
  }

  if (target_time_ms >= snapshots.back().timestamp_ms) {
    return snapshots.back().position;
  }

  auto upper = std::lower_bound(
      snapshots.begin(), snapshots.end(), target_time_ms,
      [](const PlayerSnapshot& s, std::uint32_t t) {
        std::int32_t diff = static_cast<std::int32_t>(t - s.timestamp_ms);
        return diff > 0;
      });

  if (upper == snapshots.begin()) {
    return upper->position;
  }

  auto lower = std::prev(upper);

  const auto& p0 = lower->position;
  const auto& p1 = upper->position;
  std::uint32_t t0 = lower->timestamp_ms;
  std::uint32_t t1 = upper->timestamp_ms;

  if (t1 == t0) {
    return p0;
  }

  float alpha =
      static_cast<float>(target_time_ms - t0) / static_cast<float>(t1 - t0);

  return engine::math::Vector2f{p0.x + (p1.x - p0.x) * alpha,
                                p0.y + (p1.y - p0.y) * alpha};
}

}  // namespace server
