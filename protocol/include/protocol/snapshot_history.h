#ifndef PROTOCOL_SNAPSHOT_HISTORY_H_
#define PROTOCOL_SNAPSHOT_HISTORY_H_

#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <vector>

#include "protocol/world_snapshot.h"

namespace protocol {

  /**
   * @brief Rolling history of world snapshots for delta compression and interpolation.
   * 
   * Maintains a fixed-size window of recent snapshots, allowing:
   *   - Delta compression by referencing previous snapshots
   *   - Client-side interpolation between snapshots
   *   - Snapshot lookup by ID for acknowledgment handling
   */
  class SnapshotHistory {
    public:
      /**
       * @brief Constructs a SnapshotHistory with a specified capacity.
       * @param max_snapshots Maximum number of snapshots to retain in the history.
       * 
       * When the history is full, the oldest snapshot is removed when adding a new one.
       */
      explicit SnapshotHistory(std::size_t max_snapshots);

      /**
       * @brief Adds a new snapshot to the history.
       * @param snapshot The world snapshot to add.
       * 
       * If the history is at capacity, the oldest snapshot is removed first.
       * Snapshots should be added in chronological order (increasing snapshot_id).
       */
      void AddSnapshot(const WorldSnapshotPayload& snapshot);

      /**
       * @brief Retrieves a snapshot by its ID.
       * @param snapshot_id The ID of the snapshot to retrieve.
       * @return Snapshot reference if found.
       * 
       * Performs a linear search through the history. Use sparingly in performance-critical code.
       */
      std::optional<std::reference_wrapper<const WorldSnapshotPayload>> GetSnapshot(
          std::uint32_t snapshot_id) const;

      /**
       * @brief Retrieves the most recently added snapshot.
       * @return Snapshot reference for the latest entry if present.
       */
      std::optional<std::reference_wrapper<const WorldSnapshotPayload>> GetLatestSnapshot()
          const;

      /**
       * @brief Checks if a snapshot with the given ID exists in the history.
       * @param snapshot_id The ID to check for.
       * @return true if the snapshot exists in the history, false otherwise.
       */
      bool Contains(std::uint32_t snapshot_id) const;

      /**
       * @brief Returns the current number of snapshots in the history.
       * @return Number of stored snapshots.
       */
      std::size_t size() const { return snapshots_.size(); }

      /**
       * @brief Returns the maximum capacity of the history.
       * @return Maximum number of snapshots that can be stored.
       */
      std::size_t capacity() const { return max_snapshots_; }

    private:
      std::size_t max_snapshots_;  ///< Maximum number of snapshots to retain.
      std::vector<WorldSnapshotPayload> snapshots_;  ///< Stored snapshots.
  };

}

#endif // PROTOCOL_SNAPSHOT_HISTORY_H_
