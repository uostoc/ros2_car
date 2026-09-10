#pragma once

#include <array>
#include <optional>

#include <QList>
#include <QSet>
#include <QString>

namespace car_operator_console {

class TeleopKeyBindings final {
 public:
  enum Action { kForward, kReverse, kLeft, kRight, kStop, kActionCount };

  TeleopKeyBindings();

  void load();
  void save() const;
  void reset();
  bool set_custom_key(Action action, int key, QString *error = nullptr);
  int custom_key(Action action) const;
  QList<int> keys_for(Action action) const;
  std::optional<Action> action_for_key(int key) const;
  bool is_pressed(Action action, const QSet<int> &pressed_keys) const;
  static bool is_assignable_key(int key);

 private:
  static QList<int> default_keys(Action action);

  std::array<int, kActionCount> custom_keys_{};
};

}  // namespace car_operator_console
