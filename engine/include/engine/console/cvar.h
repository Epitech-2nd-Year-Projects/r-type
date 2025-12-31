#ifndef ENGINE_CONSOLE_CVAR_H_
#define ENGINE_CONSOLE_CVAR_H_

#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <variant>

namespace engine::console {

enum class CVarFlags : std::uint32_t {
  kNone = 0,
  kReadOnly = 1 << 0,
  kCheat = 1 << 1,
  kArchive = 1 << 2,
  kHidden = 1 << 3,
};

inline CVarFlags operator|(CVarFlags a, CVarFlags b) {
  return static_cast<CVarFlags>(static_cast<std::uint32_t>(a) |
                                static_cast<std::uint32_t>(b));
}

inline CVarFlags operator&(CVarFlags a, CVarFlags b) {
  return static_cast<CVarFlags>(static_cast<std::uint32_t>(a) &
                                static_cast<std::uint32_t>(b));
}

inline bool HasFlag(CVarFlags flags, CVarFlags flag) {
  return (static_cast<std::uint32_t>(flags) &
          static_cast<std::uint32_t>(flag)) != 0;
}

using CVarValue = std::variant<bool, int, float, std::string>;

class CVarBase {
 public:
  CVarBase(std::string name, std::string description, CVarFlags flags);
  virtual ~CVarBase() = default;

  CVarBase(const CVarBase&) = delete;
  CVarBase& operator=(const CVarBase&) = delete;
  CVarBase(CVarBase&&) = default;
  CVarBase& operator=(CVarBase&&) = default;

  const std::string& name() const { return name_; }
  const std::string& description() const { return description_; }
  CVarFlags flags() const { return flags_; }

  virtual std::string GetString() const = 0;
  virtual bool SetFromString(std::string_view value) = 0;
  virtual void Reset() = 0;

 protected:
  std::string name_;
  std::string description_;
  CVarFlags flags_;
};

template <typename T>
class CVar : public CVarBase {
 public:
  using ChangeCallback =
      std::function<void(const T& old_value, const T& new_value)>;

  CVar(std::string name, T default_value, std::string description = "",
       CVarFlags flags = CVarFlags::kNone);

  CVar(std::string name, T default_value, T min_value, T max_value,
       std::string description = "", CVarFlags flags = CVarFlags::kNone);

  const T& Get() const { return value_; }
  bool Set(const T& value);

  const T& GetDefault() const { return default_value_; }
  std::optional<T> GetMin() const { return min_value_; }
  std::optional<T> GetMax() const { return max_value_; }

  std::string GetString() const override;
  bool SetFromString(std::string_view value) override;
  void Reset() override;

  void OnChange(ChangeCallback callback);

 private:
  T Clamp(const T& value) const;

  T value_;
  T default_value_;
  std::optional<T> min_value_;
  std::optional<T> max_value_;
  std::vector<ChangeCallback> callbacks_;
};

template <>
std::string CVar<bool>::GetString() const;
template <>
std::string CVar<int>::GetString() const;
template <>
std::string CVar<float>::GetString() const;
template <>
std::string CVar<std::string>::GetString() const;

template <>
bool CVar<bool>::SetFromString(std::string_view value);
template <>
bool CVar<int>::SetFromString(std::string_view value);
template <>
bool CVar<float>::SetFromString(std::string_view value);
template <>
bool CVar<std::string>::SetFromString(std::string_view value);

class CVarRegistry {
 public:
  static CVarRegistry& Instance();

  template <typename T>
  CVar<T>& Register(std::string name, T default_value,
                    std::string description = "",
                    CVarFlags flags = CVarFlags::kNone);

  template <typename T>
  CVar<T>& Register(std::string name, T default_value, T min_value, T max_value,
                    std::string description = "",
                    CVarFlags flags = CVarFlags::kNone);

  CVarBase* Find(std::string_view name);
  const CVarBase* Find(std::string_view name) const;

  template <typename T>
  CVar<T>* FindTyped(std::string_view name);

  bool Exists(std::string_view name) const;

  std::string GetString(std::string_view name) const;
  bool SetFromString(std::string_view name, std::string_view value);

  std::vector<std::string> GetAllNames() const;
  std::vector<std::string> GetNamesWithPrefix(std::string_view prefix) const;

  void Clear();

 private:
  CVarRegistry() = default;

  std::unordered_map<std::string, std::unique_ptr<CVarBase>> cvars_;
  mutable std::mutex mutex_;
};

extern template class CVar<bool>;
extern template class CVar<int>;
extern template class CVar<float>;
extern template class CVar<std::string>;

}  // namespace engine::console

#endif  // ENGINE_CONSOLE_CVAR_H_
