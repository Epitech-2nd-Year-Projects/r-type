#ifndef RIFT_CLIENT_INPUT_FIGHT_INPUT_H_
#define RIFT_CLIENT_INPUT_FIGHT_INPUT_H_

#include <array>
#include <cstddef>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "engine/input.h"

namespace rift::client {

enum class FightAction {
  kMoveLeft,
  kMoveRight,
  kLightAttack,
  kHeavyAttack,
  kBlock,
  kDodge
};

inline constexpr std::size_t kFightActionCount = 6;

enum class FightActionEventType { kPressed, kReleased };

struct FightActionEvent {
  FightAction action;
  FightActionEventType type;
};

struct FightActionState {
  bool move_left{false};
  bool move_right{false};
  bool light_attack{false};
  bool heavy_attack{false};
  bool block{false};
  bool dodge{false};
};

class FightInputLayer {
 public:
  explicit FightInputLayer(engine::input::InputManager& manager);

  void ApplyDefaultBindings();

  void Update();

  FightActionState state() const { return state_; }

  std::vector<FightActionEvent> ConsumeEvents();

 private:
  std::optional<FightAction> ResolveAction(std::string_view action_name) const;
  void RefreshState();

  std::reference_wrapper<engine::input::InputManager> manager_;
  std::array<std::string, kFightActionCount> action_names_{};
  std::unordered_map<std::string_view, FightAction> action_lookup_;
  FightActionState state_{};
  std::vector<FightActionEvent> events_{};
};

}  // namespace rift::client

#endif  // RIFT_CLIENT_INPUT_FIGHT_INPUT_H_
