#pragma once

namespace car_operator_console {

struct ConsoleState {
  static constexpr int kStopped = 0;
  static constexpr int kStarting = 1;
  static constexpr int kRunning = 2;
  static constexpr int kStopping = 3;
  static constexpr int kError = 4;

  int navigation_state{kStopped};

  bool can_navigate() const { return navigation_state == kRunning; }
};

}  // namespace car_operator_console
