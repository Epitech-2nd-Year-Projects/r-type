#ifndef ENGINE_ECS_SPARSE_ARRAY_H_
#define ENGINE_ECS_SPARSE_ARRAY_H_

#include <optional>
#include <vector>

namespace engine::ecs {

template <typename Component>
class SparseArray {
 public:
  using ValueType = std::optional<Component>;
  using ReferenceType = ValueType &;
  using ConstReferenceType = ValueType const &;
  using Container_t = std::vector<ValueType>;
  using SizeType = typename Container_t::SizeType;
  using Iterator = typename Container_t::Iterator;
  using ConstIterator = typename Container_t::ConstIterator;

 public:
  SparseArray() = default;
  SparseArray(SparseArray const &) = default;
  SparseArray(SparseArray &&) noexcept = default;
  ~SparseArray() = default;
  SparseArray &operator=(SparseArray const &) = default;
  SparseArray &operator=(SparseArray &&) noexcept = default;

  ReferenceType operator[](size_t idx) { return data_[idx]; }
  ConstReferenceType operator[](size_t idx) const { return data_[idx]; }
  Iterator Begin() { return data_.begin(); };
  ConstIterator Begin() const { return data_.begin(); };
  ConstIterator CBegin() const { return data_.cbegin(); };

  Iterator End() { return data_.end(); };
  ConstIterator End() const { return data_.end(); };
  ConstIterator CEnd() const { return data_.cend(); };

  SizeType Size() const { return data_.size(); };

  ReferenceType InsertAt(SizeType pos, Component const &comp) {
    if (pos >= data_.size()) data_.resize(pos + 1);
    data_[pos] = comp;
    return data_[pos];
  }
  ReferenceType InsertAt(SizeType pos, Component &&comp) {
    if (pos >= data_.size()) data_.resize(pos + 1);
    data_[pos] = std::move(comp);
    return data_[pos];
  };

  template <class... Params>
  ReferenceType EmplaceAt(SizeType pos, Params &&...param) {
    if (pos >= data_.size()) data_.resize(pos + 1);
    data_[pos].emplace(std::forward<Params>(param)...);
    return data_[pos];
  };

  void Erase(SizeType pos) {
    if (pos < data_.size()) data_[pos] = std::nullopt;
  };

  SizeType GetIndex(ValueType const &val) const {
    return std::distance(data_.data(), std::addressof(val));
  };

 private:
  Container_t data_;
};

}  // namespace engine::ecs

#endif /* !ENGINE_ECS_SPARSE_ARRAY_H_ */