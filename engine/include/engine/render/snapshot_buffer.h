#ifndef ENGINE_RENDER_SNAPSHOT_BUFFER_H_
#define ENGINE_RENDER_SNAPSHOT_BUFFER_H_

#include <array>
#include <atomic>
#include <cstddef>
#include <utility>

#include "engine/render/render_snapshot.h"

namespace engine::render {

/**
 * @brief Thread-safe triple buffer for RenderSnapshot exchange.
 *
 * @details
 * Allows the logic thread to produce snapshots and the render thread
 * to consume them without locking. Supports interpolation by keeping
 * history.
 */
class SnapshotBuffer {
 public:
  SnapshotBuffer() = default;
  ~SnapshotBuffer() = default;

  SnapshotBuffer(const SnapshotBuffer&) = delete;
  SnapshotBuffer& operator=(const SnapshotBuffer&) = delete;

  /**
   * @brief Submit a new snapshot from the logic thread.
   * @param snapshot The snapshot to publish.
   */
  void Produce(RenderSnapshot&& snapshot);

  /**
   * @brief Get the latest available snapshot for rendering.
   * @return Reference to the latest snapshot.
   */
  const RenderSnapshot& Consume();

  /**
   * @brief Get the pair of snapshots for interpolation.
   * @return Pair {previous, current} snapshots.
   */
  std::pair<const RenderSnapshot&, const RenderSnapshot&>
  GetInterpolationPair();

 private:
  static constexpr std::size_t kBufferSize = 3;

  std::array<RenderSnapshot, kBufferSize> buffers_;

  // Index where the producer is currently writing or will write next
  std::size_t write_index_{0};

  // Index of the most recently fully written snapshot (published)
  std::atomic<std::size_t> ready_index_{0};

  // Index currently being read by the consumer (render thread)
  std::size_t read_index_{0};

  // Previous index read by consumer (for interpolation)
  std::size_t prev_read_index_{0};
};

}  // namespace engine::render

#endif  // ENGINE_RENDER_SNAPSHOT_BUFFER_H_
