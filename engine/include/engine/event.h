#ifndef ENGINE_ENGINE_EVENT_H_
#define ENGINE_ENGINE_EVENT_H_

#include <cstddef>
#include <functional>
#include <memory>
#include <mutex>
#include <type_traits>
#include <typeindex>
#include <typeinfo>
#include <unordered_map>
#include <utility>
#include <vector>

namespace engine::event {

/**
 * @brief Lightweight event bus used to decouple systems.
 *
 * Systems register typed callbacks and emit events without referencing each
 * other directly. Events can either be dispatched immediately or queued and
 * flushed later (useful when systems run in different phases).
 */
class EventBus {
 public:
  /**
   * @brief Handle returned by Subscribe to later remove a listener.
   */
  struct SubscriptionHandle {
    const std::type_info* type = nullptr;
    std::size_t id = 0;

    [[nodiscard]] bool Valid() const noexcept {
      return type != nullptr && id != 0;
    }

    explicit operator bool() const noexcept { return Valid(); }
  };

  EventBus() = default;
  EventBus(const EventBus&) = delete;
  EventBus& operator=(const EventBus&) = delete;
  EventBus(EventBus&&) = delete;
  EventBus& operator=(EventBus&&) = delete;

  /**
   * @brief Subscribe to a typed event.
   *
   * @tparam Event     Struct/class describing the event payload.
   * @tparam Callable  Any callable taking (const Event&).
   * @return Handle that can later be used with Unsubscribe.
   */
  template <typename Event, typename Callable>
  SubscriptionHandle Subscribe(Callable&& callback);

  /**
   * @brief Remove a previously registered listener.
   */
  void Unsubscribe(const SubscriptionHandle& handle);

  /**
   * @brief Publish an event immediately to every subscriber of the same type.
   */
  template <typename Event>
  void Publish(const Event& event);

  /**
   * @brief Publish an event by value, leveraging move semantics.
   */
  template <typename Event>
  void Publish(Event&& event);

  /**
   * @brief Construct and publish an event in-place.
   */
  template <typename Event, typename... Args>
  void Emplace(Args&&... args);

  /**
   * @brief Enqueue an event to be dispatched later via DispatchQueued.
   */
  template <typename Event, typename... Args>
  void Enqueue(Args&&... args);

  /**
   * @brief Dispatch every queued event in FIFO order.
   */
  void DispatchQueued();

  /**
   * @brief Check whether pending events are waiting to be processed.
   */
  [[nodiscard]] bool HasQueuedEvents() const;

  /**
   * @brief Remove every subscription and pending event.
   */
  void Clear();

 private:
  struct HandlerBase {
    virtual ~HandlerBase() = default;
    virtual void Invoke(const void* event) const = 0;
    [[nodiscard]] virtual std::size_t Id() const noexcept = 0;
  };

  template <typename Event>
  struct TypedHandler : HandlerBase {
    TypedHandler(std::size_t handler_id,
                 std::function<void(const Event&)> callback_fn)
        : id(handler_id), callback(std::move(callback_fn)) {}

    void Invoke(const void* event) const override {
      callback(*static_cast<const Event*>(event));
    }

    [[nodiscard]] std::size_t Id() const noexcept override { return id; }

    std::size_t id;
    std::function<void(const Event&)> callback;
  };

  using TypeId = std::type_index;
  using HandlerList = std::vector<std::shared_ptr<HandlerBase>>;

  template <typename Event>
  void PublishImpl(const Event& event);

  std::unordered_map<TypeId, HandlerList> subscribers_;
  mutable std::mutex subscribers_mutex_;

  std::vector<std::function<void()>> queued_events_;
  mutable std::mutex queue_mutex_;

  std::size_t next_subscription_id_ = 1;
};

template <typename Event, typename Callable>
EventBus::SubscriptionHandle EventBus::Subscribe(Callable&& callback) {
  using DecayedEvent = std::decay_t<Event>;
  const auto& type = typeid(DecayedEvent);
  auto fn = std::function<void(const DecayedEvent&)>(
      std::forward<Callable>(callback));

  std::lock_guard lock(subscribers_mutex_);
  const std::size_t id = next_subscription_id_++;
  auto handler =
      std::make_shared<TypedHandler<DecayedEvent>>(id, std::move(fn));
  subscribers_[TypeId(type)].push_back(std::move(handler));
  return SubscriptionHandle{&type, id};
}

template <typename Event>
void EventBus::Publish(const Event& event) {
  PublishImpl<Event>(event);
}

template <typename Event>
void EventBus::Publish(Event&& event) {
  using DecayedEvent = std::decay_t<Event>;
  DecayedEvent copy = std::forward<Event>(event);
  PublishImpl<DecayedEvent>(copy);
}

template <typename Event, typename... Args>
void EventBus::Emplace(Args&&... args) {
  Event event{std::forward<Args>(args)...};
  Publish(event);
}

template <typename Event, typename... Args>
void EventBus::Enqueue(Args&&... args) {
  using DecayedEvent = std::decay_t<Event>;
  auto event = std::make_shared<DecayedEvent>(std::forward<Args>(args)...);

  auto dispatch = [this, event]() { this->Publish(*event); };

  std::lock_guard lock(queue_mutex_);
  queued_events_.push_back(std::move(dispatch));
}

template <typename Event>
void EventBus::PublishImpl(const Event& event) {
  const auto type = TypeId(typeid(Event));
  HandlerList handlers;
  {
    std::lock_guard lock(subscribers_mutex_);
    auto it = subscribers_.find(type);
    if (it == subscribers_.end()) return;
    handlers = it->second;
  }

  for (const auto& handler : handlers) {
    handler->Invoke(&event);
  }
}

}  // namespace engine::event

#endif /* !ENGINE_ENGINE_EVENT_H_ */
