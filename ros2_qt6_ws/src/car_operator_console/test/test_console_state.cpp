#include <QtTest/QtTest>

#include "car_operator_console/console_state.hpp"

class ConsoleStateTest final : public QObject {
  Q_OBJECT

 private slots:
  void allows_navigation_only_when_navigation_stack_is_running() {
    car_operator_console::ConsoleState state;
    QVERIFY(!state.can_navigate());
    state.navigation_state = car_operator_console::ConsoleState::kRunning;
    QVERIFY(state.can_navigate());
    state.navigation_state = car_operator_console::ConsoleState::kError;
    QVERIFY(!state.can_navigate());
  }

  void blocks_repeated_mode_starts_while_a_transition_is_in_progress() {
    car_operator_console::ConsoleState state;
    state.navigation_state = car_operator_console::ConsoleState::kStarting;
    QVERIFY(!state.can_start_navigation());
    state.navigation_state = car_operator_console::ConsoleState::kRunning;
    QVERIFY(!state.can_start_navigation());
    state.navigation_state = car_operator_console::ConsoleState::kError;
    QVERIFY(state.can_start_navigation());
    state.mapping_state = car_operator_console::ConsoleState::kStopping;
    QVERIFY(!state.can_start_mapping());
    state.mapping_state = car_operator_console::ConsoleState::kRunning;
    QVERIFY(state.is_mapping());
  }
};

QTEST_APPLESS_MAIN(ConsoleStateTest)
#include "test_console_state.moc"
