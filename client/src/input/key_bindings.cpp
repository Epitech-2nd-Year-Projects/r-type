#include "input/key_bindings.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>
#include <sstream>
#include <string>
#include <unordered_map>

#include "constants/input_constants.h"
#include "engine/input/key_mappings.h"
#include "logging.h"

namespace client {
namespace {

constexpr std::array<GameAction, kGameActionCount> kActionOrder{
    GameAction::kMoveUp,    GameAction::kMoveDown, GameAction::kMoveLeft,
    GameAction::kMoveRight, GameAction::kShoot,    GameAction::kBigShoot,
    GameAction::kReconnect};

std::string ToLower(std::string_view text) {
  std::string normalized;
  normalized.reserve(text.size());
  for (unsigned char c : text) {
    normalized.push_back(
        static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
  }
  return normalized;
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
  bindings.Set(GameAction::kMoveUp, engine::input::Key::kZ);
  bindings.Add(GameAction::kMoveUp, engine::input::Key::kUp);

  bindings.Set(GameAction::kMoveDown, engine::input::Key::kS);
  bindings.Add(GameAction::kMoveDown, engine::input::Key::kDown);

  bindings.Set(GameAction::kMoveLeft, engine::input::Key::kQ);
  bindings.Add(GameAction::kMoveLeft, engine::input::Key::kLeft);

  bindings.Set(GameAction::kMoveRight, engine::input::Key::kD);
  bindings.Add(GameAction::kMoveRight, engine::input::Key::kRight);

  bindings.Set(GameAction::kShoot, engine::input::Key::kSpace);
  bindings.Set(GameAction::kBigShoot, engine::input::Key::kLeftShift);
  bindings.Set(GameAction::kReconnect, engine::input::Key::kR);
  return bindings;
}

bool KeyBindings::LoadFromFile(const std::filesystem::path& path) {
  return LoadFromJson(path);
}

bool KeyBindings::SaveToFile(const std::filesystem::path& path) const {
  const auto parent = path.parent_path();
  if (!parent.empty()) {
    std::error_code ec;
    std::filesystem::create_directories(parent, ec);
  }

  const std::string lowered_ext = ToLower(path.extension().string());
  if (lowered_ext == ".json") {
    nlohmann::json doc = nlohmann::json::object();
    for (GameAction action : kActionOrder) {
      doc[std::string(ActionToken(action))] = KeyToToken(Primary(action));
    }
    std::ofstream file(path);
    if (!file.is_open()) return false;
    file << doc.dump(2);
    return true;
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

void KeyBindings::Add(GameAction action, engine::input::Key key) {
  auto& keys = bindings_[ActionIndex(action)];
  if (std::find(keys.begin(), keys.end(), key) == keys.end()) {
    keys.push_back(key);
  }
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
    case GameAction::kBigShoot:
      return "Big Shoot";
    case GameAction::kReconnect:
      return "Reconnect";
  }
  return "Unknown";
}

std::string_view ActionToken(GameAction action) {
  switch (action) {
    case GameAction::kMoveUp:
      return constants::input::kActionMoveUp;
    case GameAction::kMoveDown:
      return constants::input::kActionMoveDown;
    case GameAction::kMoveLeft:
      return constants::input::kActionMoveLeft;
    case GameAction::kMoveRight:
      return constants::input::kActionMoveRight;
    case GameAction::kShoot:
      return constants::input::kActionShoot;
    case GameAction::kBigShoot:
      return constants::input::kActionBigShoot;
    case GameAction::kReconnect:
      return constants::input::kActionReconnect;
  }
  return "Unknown";
}

std::string KeyToToken(engine::input::Key key) {
  const auto token = engine::input::KeyToken(key);
  if (!token.empty()) {
    return std::string(token);
  }
  return "Unknown";
}

std::string KeyDisplayName(engine::input::Key key) {
  const auto display = engine::input::KeyDisplayName(key);
  if (!display.empty()) {
    return std::string(display);
  }
  return "Unknown";
}

std::optional<engine::input::Key> ParseKeyToken(std::string_view token) {
  const auto& table =
      []() -> const std::unordered_map<std::string, engine::input::Key>& {
    static const std::unordered_map<std::string, engine::input::Key> map = [] {
      std::unordered_map<std::string, engine::input::Key> built;
      for (const auto& entry : engine::input::KeyMappings()) {
        built.emplace(ToLower(entry.token), entry.key);
      }
      return built;
    }();
    return map;
  }();

  const std::string normalized = ToLower(token);
  if (const auto it = table.find(normalized); it != table.end()) {
    return it->second;
  }
  return std::nullopt;
}

std::span<const engine::input::Key> BindableKeys() {
  static const std::vector<engine::input::Key> keys = [] {
    std::vector<engine::input::Key> built;
    built.reserve(engine::input::KeyMappings().size());
    for (const auto& entry : engine::input::KeyMappings()) {
      built.push_back(entry.key);
    }
    return built;
  }();
  return keys;
}

bool KeyBindings::LoadFromJson(const std::filesystem::path& path) {
  std::ifstream file(path);
  if (!file.is_open()) return false;

  std::stringstream buffer;
  buffer << file.rdbuf();

  nlohmann::json doc;
  try {
    doc = nlohmann::json::parse(buffer.str());
  } catch (const std::exception& e) {
    LogLifecycle(engine::util::LogLevel::kWarn,
                 std::string("Failed to parse keybindings json: ") + e.what());
    return false;
  }

  if (!doc.is_object()) {
    LogLifecycle(engine::util::LogLevel::kWarn,
                 "Keybindings json root is not an object");
    return false;
  }

  for (const auto& [name, value] : doc.items()) {
    const auto action = ActionTokenToAction(name);
    if (!action) {
      LogLifecycle(engine::util::LogLevel::kWarn,
                   "Ignoring unknown action in keybindings: " + name);
      continue;
    }
    if (!value.is_string()) {
      LogLifecycle(engine::util::LogLevel::kWarn,
                   "Ignoring non-string key binding for action: " + name);
      continue;
    }
    const auto key = ParseKeyToken(value.get<std::string>());
    if (key) {
      Set(*action, *key);
    } else {
      LogLifecycle(engine::util::LogLevel::kWarn,
                   "Ignoring unknown key in keybindings: " + value.dump());
    }
  }
  return true;
}

}  // namespace client
