#include <gtest/gtest.h>

#include <sol/sol.hpp>

#include "engine/audio/audio_engine.h"
#include "engine/input.h"
#include "engine/scripting/script_engine.h"

class MockAudioEngine : public engine::audio::AudioEngine {
 public:
  void PlaySoundEffect(const std::string& path) override {
    last_sound_played = path;
  }
  void PlayMusic(const std::string& path) override {}
  void StopMusic() override {}
  void Update() override {}
  void SetMasterVolume(float volume) override {}
  void SetMusicVolume(float volume) override {}
  void SetSfxVolume(float volume) override {}
  float GetMasterVolume() const override { return 1.0f; }
  float GetMusicVolume() const override { return 1.0f; }
  float GetSfxVolume() const override { return 1.0f; }

  std::string last_sound_played;
};

class MockInputManager : public engine::input::InputManager {};

TEST(ScriptingTest, InputBindings) {
  engine::scripting::ScriptEngine script_engine;
  script_engine.Initialize();

  engine::input::InputManager input_manager;
  script_engine.SetInputManager(input_manager);

  script_engine.LuaState().script(R"(
    if Key.Space == nil then error("Key.Space is nil") end
  )");

  input_manager.BindKey("Jump", engine::input::Key::kSpace);

  input_manager.HandleKey(engine::input::Key::kSpace, true);

  bool is_space_down = script_engine.LuaState().script(R"(
    return Input:is_key_down(Key.Space)
  )");

  EXPECT_TRUE(is_space_down);

  bool is_jump_active = script_engine.LuaState().script(R"(
    return Input:is_action_active("Jump")
  )");

  EXPECT_TRUE(is_jump_active);
}

TEST(ScriptingTest, AudioBindings) {
  engine::scripting::ScriptEngine script_engine;
  script_engine.Initialize();

  MockAudioEngine audio_engine;
  script_engine.SetAudioEngine(audio_engine);

  script_engine.LuaState().script(R"(
    Audio:play_sound("bang.wav")
  )");

  EXPECT_EQ(audio_engine.last_sound_played, "bang.wav");
}
