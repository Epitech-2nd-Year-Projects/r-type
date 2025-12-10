#include "protocol/snapshot_history.h"

namespace protocol {

SnapshotHistory::SnapshotHistory(std::size_t max_snapshots)
    : max_snapshots_(max_snapshots), snapshots_() {
  snapshots_.reserve(max_snapshots_);
}

void SnapshotHistory::AddSnapshot(const WorldSnapshotPayload& snapshot) {
  if (snapshots_.size() >= max_snapshots_) {
    snapshots_.erase(snapshots_.begin());
  }
  snapshots_.push_back(snapshot);
}

std::optional<std::reference_wrapper<const WorldSnapshotPayload>>
SnapshotHistory::GetSnapshot(std::uint32_t snapshot_id) const {
  for (const auto& snapshot : snapshots_) {
    if (snapshot.snapshot_id == snapshot_id) {
      return std::cref(snapshot);
    }
  }
  return std::nullopt;
}

std::optional<std::reference_wrapper<const WorldSnapshotPayload>>
SnapshotHistory::GetLatestSnapshot() const {
  if (snapshots_.empty()) {
    return std::nullopt;
  }
  return std::cref(snapshots_.back());
}

bool SnapshotHistory::Contains(std::uint32_t snapshot_id) const {
  return GetSnapshot(snapshot_id).has_value();
}
}  // namespace protocol
