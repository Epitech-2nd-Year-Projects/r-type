#include "engine/render/snapshot_buffer.h"

#include <utility>

namespace engine::render {

void SnapshotBuffer::Produce(RenderSnapshot&& snapshot) {
  buffers_[write_index_] = std::move(snapshot);
  ready_index_.store(write_index_, std::memory_order_release);
  write_index_ = (write_index_ + 1) % kBufferSize;

  if (write_index_ == ready_index_.load(std::memory_order_acquire)) {
    write_index_ = (write_index_ + 1) % kBufferSize;
  }
}

const RenderSnapshot& SnapshotBuffer::Consume() {
  std::size_t next_ready = ready_index_.load(std::memory_order_acquire);

  if (next_ready != read_index_) {
    prev_read_index_ = read_index_;
    read_index_ = next_ready;
  }

  return buffers_[read_index_];
}

std::pair<const RenderSnapshot&, const RenderSnapshot&>
SnapshotBuffer::GetInterpolationPair() {
  Consume();
  return {buffers_[prev_read_index_], buffers_[read_index_]};
}

}  // namespace engine::render
