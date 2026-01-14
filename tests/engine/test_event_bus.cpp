#include <gtest/gtest.h>

#include "engine/event.h"

struct TestEvent {
  int value;
};

struct AnotherEvent {
  float f;
};

TEST(EventBusTest, SubscriptionAndPublish) {
  engine::event::EventBus bus;
  int received_value = 0;

  [[maybe_unused]] auto handle = bus.Subscribe<TestEvent>(
      [&](const TestEvent& e) { received_value = e.value; });

  bus.Publish(TestEvent{42});
  EXPECT_EQ(received_value, 42);
}

TEST(EventBusTest, Unsubscribe) {
  engine::event::EventBus bus;
  int call_count = 0;

  auto handle =
      bus.Subscribe<TestEvent>([&](const TestEvent& /*e*/) { call_count++; });

  bus.Publish(TestEvent{1});
  EXPECT_EQ(call_count, 1);

  bus.Unsubscribe(handle);
  bus.Publish(TestEvent{2});
  EXPECT_EQ(call_count, 1);
}

TEST(EventBusTest, EnqueueAndDispatch) {
  engine::event::EventBus bus;
  int received_value = 0;

  bus.Subscribe<TestEvent>(
      [&](const TestEvent& e) { received_value = e.value; });

  bus.Enqueue<TestEvent>(100);
  EXPECT_EQ(received_value, 0);
  EXPECT_TRUE(bus.HasQueuedEvents());

  bus.DispatchQueued();
  EXPECT_EQ(received_value, 100);
  EXPECT_FALSE(bus.HasQueuedEvents());
}

TEST(EventBusTest, MultipleEventTypes) {
  engine::event::EventBus bus;
  bool t1_received = false;
  bool t2_received = false;

  bus.Subscribe<TestEvent>([&](const TestEvent&) { t1_received = true; });
  bus.Subscribe<AnotherEvent>([&](const AnotherEvent&) { t2_received = true; });

  bus.Publish(TestEvent{0});
  EXPECT_TRUE(t1_received);
  EXPECT_FALSE(t2_received);

  bus.Publish(AnotherEvent{0.0f});
  EXPECT_TRUE(t2_received);
}

TEST(EventBusTest, CrossThreadDispatch) {
  engine::event::EventBus bus;
  std::atomic<int> received_value{0};

  bus.Subscribe<TestEvent>(
      [&](const TestEvent& e) { received_value = e.value; });

  bus.PostTo(engine::util::ThreadId::kLogic, TestEvent{99});

  EXPECT_EQ(received_value, 0);

  bus.FlushChannel(engine::util::ThreadId::kLogic);

  EXPECT_EQ(received_value, 99);
}
