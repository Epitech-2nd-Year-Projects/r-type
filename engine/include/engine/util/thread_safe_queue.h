#ifndef ENGINE_UTIL_THREAD_SAFE_QUEUE_H_
#define ENGINE_UTIL_THREAD_SAFE_QUEUE_H_

#include <condition_variable>
#include <deque>
#include <mutex>
#include <optional>
#include <utility>

namespace engine::util {

/**
 * @brief Thread-safe queue for inter-thread communication.
 *
 * @tparam T Type of elements stored in the queue.
 */
template <typename T>
class ThreadSafeQueue {
 public:
  ThreadSafeQueue() = default;
  ~ThreadSafeQueue() { Clear(); }

  ThreadSafeQueue(const ThreadSafeQueue&) = delete;
  ThreadSafeQueue& operator=(const ThreadSafeQueue&) = delete;
  ThreadSafeQueue(ThreadSafeQueue&&) = delete;
  ThreadSafeQueue& operator=(ThreadSafeQueue&&) = delete;

  /**
   * @brief Pushes an item to the back of the queue.
   * @param item The item to push.
   */
  void Push(T item) {
    {
      std::lock_guard<std::mutex> lock(mutex_);
      queue_.push_back(std::move(item));
    }
    cond_var_.notify_one();
  }

  /**
   * @brief Tries to pop an item from the front of the queue without blocking.
   * @param out_item Reference to store the popped item.
   * @return true if an item was popped, false if the queue was empty.
   */
  bool TryPop(T& out_item) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (queue_.empty()) {
      return false;
    }
    out_item = std::move(queue_.front());
    queue_.pop_front();
    return true;
  }

  /**
   * @brief Tries to pop an item from the front of the queue without blocking.
   * @return An optional containing the item if popped, or std::nullopt if
   * empty.
   */
  std::optional<T> TryPop() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (queue_.empty()) {
      return std::nullopt;
    }
    auto item = std::move(queue_.front());
    queue_.pop_front();
    return item;
  }

  /**
   * @brief Waits for an item to be available and pops it.
   * @param out_item Reference to store the popped item.
   */
  void WaitAndPop(T& out_item) {
    std::unique_lock<std::mutex> lock(mutex_);
    cond_var_.wait(lock, [this] { return !queue_.empty(); });
    out_item = std::move(queue_.front());
    queue_.pop_front();
  }

  /**
   * @brief Waits for an item to be available with a timeout.
   * @param out_item Reference to store the popped item.
   * @param timeout The maximum duration to wait.
   * @return true if an item was popped, false if the timeout occurred.
   */
  template <typename Rep, typename Period>
  bool WaitAndPopFor(T& out_item,
                     const std::chrono::duration<Rep, Period>& timeout) {
    std::unique_lock<std::mutex> lock(mutex_);
    if (!cond_var_.wait_for(lock, timeout,
                            [this] { return !queue_.empty(); })) {
      return false;
    }
    out_item = std::move(queue_.front());
    queue_.pop_front();
    return true;
  }

  /**
   * @brief Checks if the queue is empty.
   * @return true if empty, false otherwise.
   */
  bool Empty() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return queue_.empty();
  }

  /**
   * @brief Gets the number of elements in the queue.
   * @return The number of elements.
   */
  std::size_t Size() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return queue_.size();
  }

  /**
   * @brief Clears all elements from the queue.
   */
  void Clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    queue_.clear();
  }

 private:
  std::deque<T> queue_;
  mutable std::mutex mutex_;
  std::condition_variable cond_var_;
};

}  // namespace engine::util

#endif  // ENGINE_UTIL_THREAD_SAFE_QUEUE_H_
