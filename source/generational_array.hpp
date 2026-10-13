#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <type_traits>

namespace bubble {

class Ref {
public:
  constexpr Ref(std::uint32_t index, std::uint32_t generation)
      : _index{index}, _generation{generation} {}

  constexpr auto index() const -> std::uint32_t { return _index; }
  constexpr auto generation() const -> std::uint32_t { return _generation; }

private:
  std::uint32_t _index;
  std::uint32_t _generation;
};

template <typename T, std::size_t N> class GenerationalArray {
public:
  using value_t = std::remove_cvref_t<T>;

  struct Entry {
    std::optional<value_t> value;
    std::uint32_t generation;
  };

  [[nodiscard]]
  constexpr auto add(T &&v) -> std::optional<Ref> {
    auto empty_entry_index = find_empty_entry_index();
    if (!empty_entry_index.has_value()) {
      return std::nullopt;
    }

    Entry &entry = _entries[*empty_entry_index];
    entry.value = std::move(v);
    return Ref(*empty_entry_index, entry.generation);
  }

  constexpr auto remove(Ref ref) -> void {
    auto &entry = _entries[ref.index()];
    if (ref.generation() == entry.generation) {
      entry.value.reset();
      entry.generation++;
    }
  }

private:
  [[nodiscard]]
  constexpr auto find_empty_entry_index() -> std::optional<std::size_t> {
    for (std::size_t i = 0; i < N; i++) {
      Entry &entry = _entries[i];
      if (!entry.value.has_value()) {
        return i;
      }
    }

    return std::nullopt;
  }

  std::array<Entry, N> _entries;
};

} // namespace bubble
