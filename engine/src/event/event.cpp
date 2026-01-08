#include "../../include/engine/event.h"

#include <algorithm>
#include <functional>
#include <utility>

namespace engine::event {

void EventBus::Unsubscribe(const SubscriptionHandle& handle) {
  if (!handle.Valid()) return;

  std::lock_guard lock(subscribers_mutex_);
  auto it = subscribers_.find(handle.type);
  if (it == subscribers_.end()) return;

  auto& list = it->second;
  list.erase(std::remove_if(list.begin(), list.end(),
                            [&](const auto& entry) {
                              return entry && entry->Id() == handle.id;
                            }),
             list.end());

  if (list.empty()) {
    subscribers_.erase(it);
  }
}

void EventBus::DispatchQueued() {
  std::vector<std::function<void()>> pending;
  {
    std::lock_guard lock(queue_mutex_);
    if (queued_events_.empty()) return;
    pending.swap(queued_events_);
  }

  for (auto& job : pending) {
    if (job) job();
  }
}

void EventBus::FlushChannel(engine::util::ThreadId self) {
  std::lock_guard lock(channels_mutex_);
  channels_[self].FlushEvents();
}

bool EventBus::HasQueuedEvents() const {
  std::lock_guard lock(queue_mutex_);
  return !queued_events_.empty();
}

void EventBus::Clear() {
  {
    std::lock_guard lock(subscribers_mutex_);
    subscribers_.clear();
    next_subscription_id_ = 1;
  }
  {
    std::lock_guard lock(queue_mutex_);
    queued_events_.clear();
  }
}

}  // namespace engine::event
