#include "engine/input/key_mappings.h"

#include <array>

namespace engine::input {
namespace {

constexpr std::array<KeyMapping, 57> kKeyMappings{{
    {Key::kA, "A", "A", "a", "A"},
    {Key::kB, "B", "B", "b", "B"},
    {Key::kC, "C", "C", "c", "C"},
    {Key::kD, "D", "D", "d", "D"},
    {Key::kE, "E", "E", "e", "E"},
    {Key::kF, "F", "F", "f", "F"},
    {Key::kG, "G", "G", "g", "G"},
    {Key::kH, "H", "H", "h", "H"},
    {Key::kI, "I", "I", "i", "I"},
    {Key::kJ, "J", "J", "j", "J"},
    {Key::kK, "K", "K", "k", "K"},
    {Key::kL, "L", "L", "l", "L"},
    {Key::kM, "M", "M", "m", "M"},
    {Key::kN, "N", "N", "n", "N"},
    {Key::kO, "O", "O", "o", "O"},
    {Key::kP, "P", "P", "p", "P"},
    {Key::kQ, "Q", "Q", "q", "Q"},
    {Key::kR, "R", "R", "r", "R"},
    {Key::kS, "S", "S", "s", "S"},
    {Key::kT, "T", "T", "t", "T"},
    {Key::kU, "U", "U", "u", "U"},
    {Key::kV, "V", "V", "v", "V"},
    {Key::kW, "W", "W", "w", "W"},
    {Key::kX, "X", "X", "x", "X"},
    {Key::kY, "Y", "Y", "y", "Y"},
    {Key::kZ, "Z", "Z", "z", "Z"},
    {Key::kNum0, "Num0", "\u00e0", "\u00e0", "0"},
    {Key::kNum1, "Num1", "&", "&", "1"},
    {Key::kNum2, "Num2", "\u00e9", "\u00e9", "2"},
    {Key::kNum3, "Num3", "\"", "\"", "3"},
    {Key::kNum4, "Num4", "'", "'", "4"},
    {Key::kNum5, "Num5", "(", "(", "5"},
    {Key::kNum6, "Num6", "-", "-", "6"},
    {Key::kNum7, "Num7", "\u00e8", "\u00e8", "7"},
    {Key::kNum8, "Num8", "_", "_", "8"},
    {Key::kNum9, "Num9", "\u00e7", "\u00e7", "9"},
    {Key::kPeriod, "Period", ":", ":", "/"},
    {Key::kComma, "Comma", ",", ",", "?"},
    {Key::kSlash, "Slash", "!", "!", "\u00a7"},
    {Key::kBackslash, "Backslash", "*", "*", "\u00b5"},
    {Key::kSemicolon, "Semicolon", ";", ";", "."},
    {Key::kEqual, "Equal", "=", "=", "+"},
    {Key::kMinus, "Minus", ")", ")", "\u00b0"},
    {Key::kUp, "Up", "Up", "", ""},
    {Key::kDown, "Down", "Down", "", ""},
    {Key::kLeft, "Left", "Left", "", ""},
    {Key::kRight, "Right", "Right", "", ""},
    {Key::kSpace, "Space", "Space", " ", " "},
    {Key::kBackspace, "Backspace", "Backspace", "", ""},
    {Key::kEnter, "Enter", "Enter", "", ""},
    {Key::kEscape, "Escape", "Escape", "", ""},
    {Key::kLeftShift, "LeftShift", "Left Shift", "", ""},
    {Key::kRightShift, "RightShift", "Right Shift", "", ""},
    {Key::kLeftControl, "LeftCtrl", "Left Ctrl", "", ""},
    {Key::kRightControl, "RightCtrl", "Right Ctrl", "", ""},
    {Key::kLeftAlt, "LeftAlt", "Left Alt", "", ""},
    {Key::kRightAlt, "RightAlt", "Right Alt", "", ""},
}};

}  // namespace

std::span<const KeyMapping> KeyMappings() { return kKeyMappings; }

std::string_view KeyToken(Key key) {
  for (const auto& entry : kKeyMappings) {
    if (entry.key == key) {
      return entry.token;
    }
  }
  return {};
}

std::string_view KeyDisplayName(Key key) {
  for (const auto& entry : kKeyMappings) {
    if (entry.key == key) {
      return entry.display;
    }
  }
  return {};
}

std::string_view KeyText(Key key, bool shifted) {
  for (const auto& entry : kKeyMappings) {
    if (entry.key == key) {
      return shifted ? entry.shifted_text : entry.text;
    }
  }
  return {};
}

}  // namespace engine::input
