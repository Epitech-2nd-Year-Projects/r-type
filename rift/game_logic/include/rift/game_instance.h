#ifndef RIFT_GAME_INSTANCE_H_
#define RIFT_GAME_INSTANCE_H_

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "engine/ecs/registry.h"
#include "engine/time/time_delta.h"

namespace rift {

struct GameState {
  std::uint32_t room_id{0};
  std::uint8_t round_number{1};
  std::uint8_t player1_rounds_won{0};
  std::uint8_t player2_rounds_won{0};
  bool match_over{false};
  std::uint32_t round_timer_ms{0};
};

class GameInstance {
 public:
  enum class InputEventType : std::uint8_t {
    kMoveLeftPressed = 0,
    kMoveLeftReleased = 1,
    kMoveRightPressed = 2,
    kMoveRightReleased = 3,
    kMoveUpPressed = 4,
    kMoveUpReleased = 5,
    kMoveDownPressed = 6,
    kMoveDownReleased = 7,
    kLightAttackPressed = 8,
    kLightAttackReleased = 9,
    kHeavyAttackPressed = 10,
    kHeavyAttackReleased = 11
  };

  struct PlayerDeathEvent {
    std::uint32_t player_id{0};
    std::uint8_t remaining_lives{0};
  };

  explicit GameInstance(std::uint32_t room_id, std::uint32_t max_players = 2);
  ~GameInstance();

  GameInstance(const GameInstance&) = delete;
  GameInstance& operator=(const GameInstance&) = delete;

  void Start();
  void Update(engine::time::TimeDelta dt);
  void Shutdown();

  std::optional<engine::ecs::EntityId> OnPlayerJoin(std::uint32_t player_id,
                                                     std::string_view player_name);
  void OnPlayerLeave(std::uint32_t player_id);
  void OnPlayerInput(std::uint32_t player_id, InputEventType input_type);
  void OnPlayerDeath(std::uint32_t player_id, std::uint8_t remaining_lives);

  std::vector<PlayerDeathEvent> ExtractPlayerDeathEvents();

  engine::ecs::Registry& World();
  const engine::ecs::Registry& World() const;

  const GameState& State() const;
  GameState& State();

  bool IsRunning() const;
  bool IsFinished() const;
  std::uint32_t RoomId() const;
  std::uint32_t MaxPlayers() const;
  std::uint32_t ActivePlayerCount() const;

 private:
  struct InputState {
    bool move_left{false};
    bool move_right{false};
    bool blocking{false};
    bool dodging{false};
  };

  void RegisterComponents();
  void RegisterSystems();
  void InitializeGame();
  void SpawnFighter(std::uint32_t player_id, std::uint8_t slot);

  std::uint32_t room_id_;
  std::uint32_t max_players_;
  std::unique_ptr<engine::ecs::Registry> registry_;
  GameState game_state_;
  std::unordered_map<std::uint32_t, std::string> player_names_;
  std::unordered_map<std::uint32_t, engine::ecs::EntityId> player_entities_;
  std::unordered_map<std::uint32_t, InputState> player_input_states_;
  std::vector<PlayerDeathEvent> pending_deaths_;
  bool is_started_{false};
};

}  // namespace rift

#endif  // RIFT_GAME_INSTANCE_H_
