#include <gtest/gtest.h>

#include "engine/audio/audio_command.h"
#include "engine/audio/audio_dispatcher.h"

using namespace engine::audio;

TEST(AudioDispatcherTest, PushesPlaySoundCommand) {
  AudioCommandQueue queue;
  AudioDispatcher dispatcher(queue);

  dispatcher.PlaySound("test.wav", 0.5f);

  AudioCommand cmd;
  ASSERT_TRUE(queue.TryPop(cmd));
  ASSERT_TRUE(std::holds_alternative<PlaySoundCommand>(cmd));

  const auto& play_cmd = std::get<PlaySoundCommand>(cmd);
  EXPECT_EQ(play_cmd.path, "test.wav");
  EXPECT_EQ(play_cmd.volume, 0.5f);
}

TEST(AudioDispatcherTest, PushesPlayMusicCommand) {
  AudioCommandQueue queue;
  AudioDispatcher dispatcher(queue);

  dispatcher.PlayMusic("music.mp3", false);

  AudioCommand cmd;
  ASSERT_TRUE(queue.TryPop(cmd));
  ASSERT_TRUE(std::holds_alternative<PlayMusicCommand>(cmd));

  const auto& play_cmd = std::get<PlayMusicCommand>(cmd);
  EXPECT_EQ(play_cmd.path, "music.mp3");
  EXPECT_FALSE(play_cmd.looping);
}

TEST(AudioDispatcherTest, PushesVolumeCommands) {
  AudioCommandQueue queue;
  AudioDispatcher dispatcher(queue);

  dispatcher.SetMasterVolume(0.8f);

  AudioCommand cmd;
  ASSERT_TRUE(queue.TryPop(cmd));
  ASSERT_TRUE(std::holds_alternative<SetVolumeCommand>(cmd));

  const auto& vol_cmd = std::get<SetVolumeCommand>(cmd);
  EXPECT_EQ(vol_cmd.target, SetVolumeCommand::Target::kMaster);
  EXPECT_EQ(vol_cmd.volume, 0.8f);
}
