#include "key_bindings.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <string>

namespace client {
namespace {

constexpr std::string_view kConfigCommentStart = "#";
constexpr std::string_view kSeparator = "=";

struct KeyName {
  engine::input::Key key;
  std::string_view token;
  std::string_view display;
};

constexpr std::array<KeyName, 57> kKeyNames{{
    {engine::input::Key::kA, "A", "A"},
    {engine::input::Key::kB, "B", "B"},
    {engine::input::Key::kC, "C", "C"},
    {engine::input::Key::kD, "D", "D"},
    {engine::input::Key::kE, "E", "E"},
    {engine::input::Key::kF, "F", "F"},
    {engine::input::Key::kG, "G", "G"},
    {engine::input::Key::kH, "H", "H"},
    {engine::input::Key::kI, "I", "I"},
    {engine::input::Key::kJ, "J", "J"},
    {engine::input::Key::kK, "K", "K"},
    {engine::input::Key::kL, "L", "L"},
    {engine::input::Key::kM, "M", "M"},
    {engine::input::Key::kN, "N", "N"},
    {engine::input::Key::kO, "O", "O"},
    {engine::input::Key::kP, "P", "P"},
    {engine::input::Key::kQ, "Q", "Q"},
    {engine::input::Key::kR, "R", "R"},
    {engine::input::Key::kS, "S", "S"},
    {engine::input::Key::kT, "T", "T"},
    {engine::input::Key::kU, "U", "U"},
    {engine::input::Key::kV, "V", "V"},
    {engine::input::Key::kW, "W", "W"},
    {engine::input::Key::kX, "X", "X"},
    {engine::input::Key::kY, "Y", "Y"},
    {engine::input::Key::kZ, "Z", "Z"},
    {engine::input::Key::kNum0, "Num0", "0"},
    {engine::input::Key::kNum1, "Num1", "1"},
    {engine::input::Key::kNum2, "Num2", "2"},
    {engine::input::Key::kNum3, "Num3", "3"},
    {engine::input::Key::kNum4, "Num4", "4"},
    {engine::input::Key::kNum5, "Num5", "5"},
    {engine::input::Key::kNum6, "Num6", "6"},
    {engine::input::Key::kNum7, "Num7", "7"},
    {engine::input::Key::kNum8, "Num8", "8"},
    {engine::input::Key::kNum9, "Num9", "9"},
    {engine::input::Key::kPeriod, "Period", "."},
    {engine::input::Key::kComma, "Comma", ","},
    {engine::input::Key::kSlash, "Slash", "/"},
    {engine::input::Key::kBackslash, "Backslash", "\\"},
    {engine::input::Key::kSemicolon, "Semicolon", ";"},
    {engine::input::Key::kEqual, "Equal", "="},
    {engine::input::Key::kMinus, "Minus", "-"},
    {engine::input::Key::kUp, "Up", "Up"},
    {engine::input::Key::kDown, "Down", "Down"},
    {engine::input::Key::kLeft, "Left", "Left"},
    {engine::input::Key::kRight, "Right", "Right"},
    {engine::input::Key::kSpace, "Space", "Space"},
    {engine::input::Key::kBackspace, "Backspace", "Backspace"},
    {engine::input::Key::kEnter, "Enter", "Enter"},
    {engine::input::Key::kEscape, "Escape", "Escape"},
    {engine::input::Key::kLeftShift, "LeftShift", "Left Shift"},
    {engine::input::Key::kRightShift, "RightShift", "Right Shift"},
    {engine::input::Key::kLeftControl, "LeftCtrl", "Left Ctrl"},
    {engine::input::Key::kRightControl, "RightCtrl", "Right Ctrl"},
    {engine::input::Key::kLeftAlt, "LeftAlt", "Left Alt"},
    {engine::input::Key::kRightAlt, "RightAlt", "Right Alt"},
}};

constexpr std::array<GameAction, 6> kActionOrder{
    GameAction::kMoveUp,   GameAction::kMoveDown, GameAction::kMoveLeft,
    GameAction::kMoveRight, GameAction::kShoot,   GameAction::kReconnect};

std::string ToLower(std::string_view text) {
  std::string normalized;
  normalized.reserve(text.size());
  for (unsigned char c : text) {
    normalized.push_back(
        static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
  }
  return normalized;
}

std::string_view Trim(std::string_view text) {
  constexpr std::string_view kWhitespace = " \t\r\n";
  const auto begin = text.find_first_not_of(kWhitespace);
  if (begin == std::string_view::npos) return {};
  const auto end = text.find_last_not_of(kWhitespace);
  return text.substr(begin, end - begin + 1);
}

std::optional<GameAction> ActionTokenToAction(std::string_view token) {
  const std::string normalized = ToLower(token);
  for (GameAction action : kActionOrder) {
    if (ToLower(ActionToken(action)) == normalized) {
      return action;
    }
  }
  return std::nullopt;
}

}  // namespace

KeyBindings KeyBindings::Default() {
  KeyBindings bindings;
  bindings.Set(GameAction::kMoveUp, engine::input::Key::kW);
  bindings.bindings_[ActionIndex(GameAction::kMoveUp)].push_back(
      engine::input::Key::kZ);
  bindings.bindings_[ActionIndex(GameAction::kMoveUp)].push_back(
      engine::input::Key::kUp);

  bindings.Set(GameAction::kMoveDown, engine::input::Key::kS);
  bindings.bindings_[ActionIndex(GameAction::kMoveDown)].push_back(
      engine::input::Key::kDown);

  bindings.Set(GameAction::kMoveLeft, engine::input::Key::kA);
  bindings.bindings_[ActionIndex(GameAction::kMoveLeft)].push_back(
      engine::input::Key::kQ);
  bindings.bindings_[ActionIndex(GameAction::kMoveLeft)].push_back(
      engine::input::Key::kLeft);

  bindings.Set(GameAction::kMoveRight, engine::input::Key::kD);
  bindings.bindings_[ActionIndex(GameAction::kMoveRight)].push_back(
      engine::input::Key::kRight);

  bindings.Set(GameAction::kShoot, engine::input::Key::kSpace);
  bindings.Set(GameAction::kReconnect, engine::input::Key::kR);
  return bindings;
}

bool KeyBindings::LoadFromFile(const std::filesystem::path& path) {
  std::ifstream file(path);
  if (!file.is_open()) {
    return false;
  }

  std::string line;
  while (std::getline(file, line)) {
    const std::string_view trimmed = Trim(line);
    if (trimmed.empty() ||
        trimmed.rfind(kConfigCommentStart, 0) == 0 ||
        trimmed.find(kSeparator) == std::string_view::npos) {
      continue;
    }

    const auto sep = trimmed.find('=');
    const auto action_token = Trim(trimmed.substr(0, sep));
    const auto key_token = Trim(trimmed.substr(sep + 1));
    const auto action = ActionTokenToAction(action_token);
    const auto key = ParseKeyToken(key_token);
    if (action && key) {
      Set(*action, *key);
    }
  }
  return true;
}

bool KeyBindings::SaveToFile(const std::filesystem::path& path) const {
  const auto parent = path.parent_path();
  if (!parent.empty()) {
    std::error_code ec;
    std::filesystem::create_directories(parent, ec);
  }

  std::ofstream file(path);
  if (!file.is_open()) {
    return false;
  }

  for (GameAction action : kActionOrder) {
    file << ActionToken(action) << '=' << KeyToToken(Primary(action)) << '\n';
  }
  return true;
}

engine::input::Key KeyBindings::Primary(GameAction action) const {
  const auto& keys = bindings_[ActionIndex(action)];
  if (!keys.empty()) {
    return keys.front();
  }
  return engine::input::Key::kUnknown;
}

void KeyBindings::Set(GameAction action, engine::input::Key key) {
  auto& keys = bindings_[ActionIndex(action)];
  keys.clear();
  keys.push_back(key);
}

const std::vector<engine::input::Key>& KeyBindings::KeysFor(
    GameAction action) const {
  return bindings_[ActionIndex(action)];
}

std::vector<GameAction> KeyBindings::Actions() const {
  return {kActionOrder.begin(), kActionOrder.end()};
}

std::size_t KeyBindings::ActionIndex(GameAction action) {
  switch (action) {
    case GameAction::kMoveUp:
      return 0;
    case GameAction::kMoveDown:
      return 1;
    case GameAction::kMoveLeft:
      return 2;
    case GameAction::kMoveRight:
      return 3;
    case GameAction::kShoot:
      return 4;
    case GameAction::kReconnect:
      return 5;
  }
  return 0;
}

std::string ActionLabel(GameAction action) {
  switch (action) {
    case GameAction::kMoveUp:
      return "Move Up";
    case GameAction::kMoveDown:
      return "Move Down";
    case GameAction::kMoveLeft:
      return "Move Left";
    case GameAction::kMoveRight:
      return "Move Right";
    case GameAction::kShoot:
      return "Shoot";
    case GameAction::kReconnect:
      return "Reconnect";
  }
  return "Unknown";
}

std::string_view ActionToken(GameAction action) {
  switch (action) {
    case GameAction::kMoveUp:
      return "MoveUp";
    case GameAction::kMoveDown:
      return "MoveDown";
    case GameAction::kMoveLeft:
      return "MoveLeft";
    case GameAction::kMoveRight:
      return "MoveRight";
    case GameAction::kShoot:
      return "Shoot";
    case GameAction::kReconnect:
      return "Reconnect";
  }
  return "Unknown";
}

std::string KeyToToken(engine::input::Key key) {
  for (const auto& entry : kKeyNames) {
    if (entry.key == key) {
      return std::string(entry.token);
    }
  }
  return "Unknown";
}

std::string KeyDisplayName(engine::input::Key key) {
  for (const auto& entry : kKeyNames) {
    if (entry.key == key) {
      return std::string(entry.display);
    }
  }
  return "Unknown";
}

std::optional<engine::input::Key> ParseKeyToken(std::string_view token) {
  const std::string normalized = ToLower(token);
  for (const auto& entry : kKeyNames) {
    if (ToLower(entry.token) == normalized) {
      return entry.key;
    }
  }
  return std::nullopt;
}

std::span<const engine::input::Key> BindableKeys() {
  static constexpr std::array<engine::input::Key, kKeyNames.size()> keys = [] {
    std::array<engine::input::Key, kKeyNames.size()> arr{};
    for (std::size_t i = 0; i < kKeyNames.size(); ++i) {
      arr[i] = kKeyNames[i].key;
    }
    return arr;
  }();
  return keys;
}

}  // namespace client
