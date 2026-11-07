#pragma once

#include <bubble/system_concept.hpp>

#include <concepts>
#include <functional>
#include <tuple>
#include <type_traits>

namespace bubble {

template <System... Systems> class Bubble {
private:
  static constexpr auto system_count_v = sizeof...(Systems);
  static_assert(system_count_v > 0);

public:
  constexpr auto system_count() const -> std::size_t { return system_count_v; }

  constexpr auto initialize(const InitializeConfig &config) -> bool {
    return for_each_system_checked([&](auto type_identity) -> bool {
      using System = typename decltype(type_identity)::type;
      return System::initialize(config);
    });
  }

  constexpr auto destroy() -> void {
    for_each_system_reverse([](auto type_identity) {
      using System = typename decltype(type_identity)::type;
      System::destroy();
    });
  }

  constexpr auto begin_frame() -> void {
    for_each_system([](auto type_identity) {
      using System = typename decltype(type_identity)::type;
      if constexpr (SystemHasBeginFrame<System>) {
        System::begin_frame();
      }
    });
  }

  constexpr auto end_frame() -> void {
    for_each_system([](auto type_identity) {
      using System = typename decltype(type_identity)::type;
      if constexpr (SystemHasEndFrame<System>) {
        System::end_frame();
      }
    });
  }

  constexpr auto update(double deltatime) -> void {
    constexpr const auto max_deltatime = double{0.25};
    const auto safe_deltatime = std::min(max_deltatime, deltatime);
    for_each_system([&](auto type_identity) {
      using System = typename decltype(type_identity)::type;
      if constexpr (SystemHasUpdate<System>) {
        System::update(safe_deltatime);
      }
    });

    m_accumulated_time += safe_deltatime;
    while (m_accumulated_time >= m_fixed_update_deltatime) {
      m_accumulated_time -= m_fixed_update_deltatime;
      for_each_system([&](auto type_identity) {
        using System = typename decltype(type_identity)::type;
        if constexpr (SystemHasFixedUpdate<System>) {
          System::fixed_update(m_fixed_update_deltatime);
        }
      });
    }
  }

private:
  template <typename Fn>
  [[nodiscard]]
  constexpr auto for_each_system_checked(Fn &&fn) -> bool
    requires requires {
      {
        (std::invoke(std::forward<Fn>(fn), std::type_identity<Systems>{}) &&
         ...)
      } -> std::same_as<bool>;
    }
  {
    return (
        (std::invoke(std::forward<Fn>(fn), std::type_identity<Systems>{})) &&
        ...);
  }

  template <typename Fn> constexpr auto for_each_system(Fn &&fn) -> void {
    ((std::invoke(std::forward<Fn>(fn), std::type_identity<Systems>{})), ...);
  }

  template <typename Fn>
  constexpr auto for_each_system_reverse(Fn &&fn) -> void {
    [&]<std::size_t... indices>(std::index_sequence<indices...>) {
      ((std::invoke(
           std::forward<Fn>(fn),
           std::type_identity<std::tuple_element_t<system_count_v - 1 - indices,
                                                   std::tuple<Systems...>>>{})),
       ...);
    }(std::make_index_sequence<system_count_v>{});
  }

  double m_accumulated_time{0.0};
  double m_fixed_update_deltatime{1.0 / 30.0};
};

} // namespace bubble
