#include <gtest/gtest.h>

#include <chrono>
#include <thread>

#include "engine/game_runtime.h"

using namespace engine;

TEST(AudioThreadIntegrationTest, DispatchesCommandsIdeally) {
  try {
    GameRuntime::Config config;
    GameRuntime runtime(config);

    runtime.StartNetwork(0);
    runtime.Start();
    auto& dispatcher = runtime.GetAudioDispatcher();
    dispatcher.SetMasterVolume(0.5f);
    dispatcher.PlaySound("test_sound.wav");
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    runtime.Stop();
    SUCCEED();
  } catch (const std::exception& e) {
    FAIL() << "Caught exception: " << e.what();
  } catch (...) {
    FAIL() << "Caught unknown exception";
  }
}
