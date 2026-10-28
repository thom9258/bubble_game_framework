#pragma once

#include <algorithm>
#include <cstdint>
#include <initializer_list>
#include <span>
#include <vector>

namespace bubble {

constexpr auto hash_combine(std::size_t hash, std::uint32_t value)
    -> std::size_t {
  static constexpr const auto golden_ratio = std::size_t{0x9e3779b9};
  hash ^= static_cast<std::size_t>(value) + golden_ratio + (hash << 6) +
          (hash >> 2);
  return hash;
}

constexpr auto hash_combine(std::initializer_list<std::uint32_t> values)
    -> std::size_t {
  auto hash = std::size_t{0};
  for (auto value : values) {
    hash = hash_combine(hash, value);
  }

  return hash;
}

template <typename T> class FlatSortedSet {
public:
  using Value = std::remove_cvref_t<T>;
  using Key = std::size_t;

  constexpr FlatSortedSet() = default;
  constexpr ~FlatSortedSet() = default;
  constexpr FlatSortedSet(const FlatSortedSet &) = delete;
  constexpr FlatSortedSet &operator=(const FlatSortedSet &) = delete;
  constexpr FlatSortedSet(FlatSortedSet &&) = default;
  constexpr FlatSortedSet &operator=(FlatSortedSet &&) = default;

  constexpr auto insert(Key key, const Value &value) -> void;

  constexpr auto insert(Key key, Value &&value) -> void;

  [[nodiscard]]
  constexpr auto find(Key key) -> Value *;

  constexpr auto reset() -> void;

  [[nodiscard]]
  constexpr auto size() const -> std::size_t;

private:
  struct Entry {
    Key key;
    Value value;
  };

  std::vector<Entry> _entries;
};

template <typename T>
constexpr auto FlatSortedSet<T>::insert(Key key, const Value &value) -> void {
  auto sorted_location =
      std::ranges::upper_bound(_entries, key, std::less<>{}, &Entry::key);
  _entries.emplace(sorted_location, key, value);
}

template <typename T>
constexpr auto FlatSortedSet<T>::insert(Key key, Value &&value) -> void {
  auto sorted_location =
      std::ranges::upper_bound(_entries, key, std::less<>{}, &Entry::key);
  _entries.emplace(sorted_location, key, std::move(value));
}

template <typename T>
constexpr auto FlatSortedSet<T>::find(Key key) -> Value * {
  const auto found =
      std::ranges::lower_bound(_entries, key, std::less<>{}, &Entry::key);
  if (found == _entries.end() || found->key != key) {
    return nullptr;
  }

  return &(found->value);
}

template <typename T> constexpr auto FlatSortedSet<T>::reset() -> void {
  _entries.clear();
}

template <typename T>
constexpr auto FlatSortedSet<T>::size() const -> std::size_t {
  return _entries.size();
}

} // namespace bubble
