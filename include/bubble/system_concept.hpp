#pragma once

#include <bubble/initialize_config.hpp>

#include <concepts>

namespace bubble {

template <typename T>
concept System = requires(const InitializeConfig &config) {
  { T::initialize(config) } -> std::same_as<bool>;
  { T::destroy() } -> std::same_as<void>;
};

template <typename T>
concept SystemHasBeginFrame = requires() {
  { T::begin_frame() } -> std::same_as<void>;
};

template <typename T>
concept SystemHasEndFrame = requires() {
  { T::end_frame() } -> std::same_as<void>;
};

template <typename T>
concept SystemHasUpdate = requires(double deltatime) {
  { T::update(deltatime) } -> std::same_as<void>;
};

template <typename T>
concept SystemHasFixedUpdate = requires(double deltatime) {
  { T::fixed_update(deltatime) } -> std::same_as<void>;
};

} // namespace bubble
