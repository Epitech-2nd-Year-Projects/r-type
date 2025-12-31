#include "engine/console/cvar.h"

#include <algorithm>
#include <charconv>
#include <sstream>

namespace engine::console {

CVarBase::CVarBase(std::string name, std::string description, CVarFlags flags)
    : name_(std::move(name)),
      description_(std::move(description)),
      flags_(flags) {}

template <typename T>
CVar<T>::CVar(std::string name, T default_value, std::string description,
              CVarFlags flags)
    : CVarBase(std::move(name), std::move(description), flags),
      value_(default_value),
      default_value_(std::move(default_value)) {}

template <typename T>
CVar<T>::CVar(std::string name, T default_value, T min_value, T max_value,
              std::string description, CVarFlags flags)
    : CVarBase(std::move(name), std::move(description), flags),
      value_(default_value),
      default_value_(default_value),
      min_value_(min_value),
      max_value_(max_value) {
  value_ = Clamp(value_);
}

template <typename T>
bool CVar<T>::Set(const T& value) {
  if (HasFlag(flags_, CVarFlags::kReadOnly)) {
    return false;
  }

  T clamped = Clamp(value);
  if (clamped == value_) {
    return true;
  }

  T old_value = value_;
  value_ = clamped;

  for (const auto& callback : callbacks_) {
    callback(old_value, value_);
  }

  return true;
}

template <typename T>
T CVar<T>::Clamp(const T& value) const {
  T result = value;
  if (min_value_.has_value() && result < *min_value_) {
    result = *min_value_;
  }
  if (max_value_.has_value() && result > *max_value_) {
    result = *max_value_;
  }
  return result;
}

template <typename T>
void CVar<T>::Reset() {
  Set(default_value_);
}

template <typename T>
void CVar<T>::OnChange(ChangeCallback callback) {
  callbacks_.push_back(std::move(callback));
}

template <>
std::string CVar<bool>::GetString() const {
  return value_ ? "true" : "false";
}

template <>
std::string CVar<int>::GetString() const {
  return std::to_string(value_);
}

template <>
std::string CVar<float>::GetString() const {
  std::ostringstream oss;
  oss << value_;
  return oss.str();
}

template <>
std::string CVar<std::string>::GetString() const {
  return value_;
}

template <>
bool CVar<bool>::SetFromString(std::string_view value) {
  std::string lower(value);
  std::transform(lower.begin(), lower.end(), lower.begin(),
                 [](unsigned char c) { return std::tolower(c); });

  if (lower == "true" || lower == "1" || lower == "yes" || lower == "on") {
    return Set(true);
  }
  if (lower == "false" || lower == "0" || lower == "no" || lower == "off") {
    return Set(false);
  }
  return false;
}

template <>
bool CVar<int>::SetFromString(std::string_view value) {
  int result = 0;
  auto [ptr, ec] = std::from_chars(value.data(), value.data() + value.size(), result);
  if (ec != std::errc{} || ptr != value.data() + value.size()) {
    return false;
  }
  return Set(result);
}

template <>
bool CVar<float>::SetFromString(std::string_view value) {
  try {
    std::string str(value);
    float result = std::stof(str);
    return Set(result);
  } catch (...) {
    return false;
  }
}

template <>
bool CVar<std::string>::SetFromString(std::string_view value) {
  return Set(std::string(value));
}

template class CVar<bool>;
template class CVar<int>;
template class CVar<float>;
template class CVar<std::string>;

CVarRegistry& CVarRegistry::Instance() {
  static CVarRegistry instance;
  return instance;
}

template <typename T>
CVar<T>& CVarRegistry::Register(std::string name, T default_value,
                                std::string description, CVarFlags flags) {
  std::lock_guard lock(mutex_);

  auto it = cvars_.find(name);
  if (it != cvars_.end()) {
    return *static_cast<CVar<T>*>(it->second.get());
  }

  auto cvar = std::make_unique<CVar<T>>(name, std::move(default_value),
                                        std::move(description), flags);
  auto* ptr = cvar.get();
  cvars_[std::move(name)] = std::move(cvar);
  return *ptr;
}

template <typename T>
CVar<T>& CVarRegistry::Register(std::string name, T default_value, T min_value,
                                T max_value, std::string description,
                                CVarFlags flags) {
  std::lock_guard lock(mutex_);

  auto it = cvars_.find(name);
  if (it != cvars_.end()) {
    return *static_cast<CVar<T>*>(it->second.get());
  }

  auto cvar = std::make_unique<CVar<T>>(name, std::move(default_value),
                                        std::move(min_value), std::move(max_value),
                                        std::move(description), flags);
  auto* ptr = cvar.get();
  cvars_[std::move(name)] = std::move(cvar);
  return *ptr;
}

template CVar<bool>& CVarRegistry::Register(std::string, bool, std::string, CVarFlags);
template CVar<int>& CVarRegistry::Register(std::string, int, std::string, CVarFlags);
template CVar<float>& CVarRegistry::Register(std::string, float, std::string, CVarFlags);
template CVar<std::string>& CVarRegistry::Register(std::string, std::string, std::string, CVarFlags);

template CVar<int>& CVarRegistry::Register(std::string, int, int, int, std::string, CVarFlags);
template CVar<float>& CVarRegistry::Register(std::string, float, float, float, std::string, CVarFlags);

CVarBase* CVarRegistry::Find(std::string_view name) {
  std::lock_guard lock(mutex_);
  auto it = cvars_.find(std::string(name));
  return it != cvars_.end() ? it->second.get() : nullptr;
}

const CVarBase* CVarRegistry::Find(std::string_view name) const {
  std::lock_guard lock(mutex_);
  auto it = cvars_.find(std::string(name));
  return it != cvars_.end() ? it->second.get() : nullptr;
}

template <typename T>
CVar<T>* CVarRegistry::FindTyped(std::string_view name) {
  return dynamic_cast<CVar<T>*>(Find(name));
}

template CVar<bool>* CVarRegistry::FindTyped<bool>(std::string_view);
template CVar<int>* CVarRegistry::FindTyped<int>(std::string_view);
template CVar<float>* CVarRegistry::FindTyped<float>(std::string_view);
template CVar<std::string>* CVarRegistry::FindTyped<std::string>(std::string_view);

bool CVarRegistry::Exists(std::string_view name) const {
  std::lock_guard lock(mutex_);
  return cvars_.find(std::string(name)) != cvars_.end();
}

std::string CVarRegistry::GetString(std::string_view name) const {
  const auto* cvar = Find(name);
  return cvar ? cvar->GetString() : "";
}

bool CVarRegistry::SetFromString(std::string_view name, std::string_view value) {
  auto* cvar = Find(name);
  return cvar ? cvar->SetFromString(value) : false;
}

std::vector<std::string> CVarRegistry::GetAllNames() const {
  std::lock_guard lock(mutex_);
  std::vector<std::string> names;
  names.reserve(cvars_.size());
  for (const auto& [name, _] : cvars_) {
    names.push_back(name);
  }
  std::sort(names.begin(), names.end());
  return names;
}

std::vector<std::string> CVarRegistry::GetNamesWithPrefix(std::string_view prefix) const {
  std::lock_guard lock(mutex_);
  std::vector<std::string> names;
  for (const auto& [name, _] : cvars_) {
    if (name.size() >= prefix.size() &&
        name.compare(0, prefix.size(), prefix) == 0) {
      names.push_back(name);
    }
  }
  std::sort(names.begin(), names.end());
  return names;
}

void CVarRegistry::Clear() {
  std::lock_guard lock(mutex_);
  cvars_.clear();
}

}  // namespace engine::console

