#include "car_operator_console/teleop_key_bindings.hpp"

#include <QSettings>

namespace car_operator_console {

namespace {

constexpr std::array<const char *, TeleopKeyBindings::kActionCount> kSettingsNames = {
    "forward", "reverse", "left", "right", "stop"};

}  // namespace

TeleopKeyBindings::TeleopKeyBindings() { load(); }

void TeleopKeyBindings::load() {
  custom_keys_.fill(0);
  QSettings settings(QSettings::NativeFormat, QSettings::UserScope,
      QStringLiteral("ROS 2 Car"), QStringLiteral("ROS 2 Car Console"));
  settings.sync();
  for (int index = 0; index < kActionCount; ++index) {
    const QString name = QStringLiteral("teleop/key/") + QString::fromLatin1(kSettingsNames.at(index));
    const int key = settings.value(name, 0).toInt();
    if (key != 0) {
      set_custom_key(static_cast<Action>(index), key);
    }
  }
}

void TeleopKeyBindings::save() const {
  QSettings settings(QSettings::NativeFormat, QSettings::UserScope,
      QStringLiteral("ROS 2 Car"), QStringLiteral("ROS 2 Car Console"));
  settings.setValue(QStringLiteral("teleop/key/forward"), custom_keys_.at(kForward));
  settings.setValue(QStringLiteral("teleop/key/reverse"), custom_keys_.at(kReverse));
  settings.setValue(QStringLiteral("teleop/key/left"), custom_keys_.at(kLeft));
  settings.setValue(QStringLiteral("teleop/key/right"), custom_keys_.at(kRight));
  settings.setValue(QStringLiteral("teleop/key/stop"), custom_keys_.at(kStop));
  settings.sync();
}

void TeleopKeyBindings::reset() {
  custom_keys_.fill(0);
  save();
}

bool TeleopKeyBindings::set_custom_key(Action action, int key, QString *error) {
  if (!is_assignable_key(key)) {
    if (error != nullptr) {
      *error = QStringLiteral("Choose a non-modifier key");
    }
    return false;
  }
  for (int index = 0; index < kActionCount; ++index) {
    const Action other = static_cast<Action>(index);
    if (other != action && keys_for(other).contains(key)) {
      if (error != nullptr) {
        *error = QStringLiteral("That key is already assigned");
      }
      return false;
    }
  }
  custom_keys_.at(action) = key;
  save();
  return true;
}

int TeleopKeyBindings::custom_key(Action action) const { return custom_keys_.at(action); }

QList<int> TeleopKeyBindings::keys_for(Action action) const {
  QList<int> keys = default_keys(action);
  const int custom = custom_key(action);
  if (custom != 0 && !keys.contains(custom)) {
    keys.append(custom);
  }
  return keys;
}

std::optional<TeleopKeyBindings::Action> TeleopKeyBindings::action_for_key(int key) const {
  for (int index = 0; index < kActionCount; ++index) {
    const Action action = static_cast<Action>(index);
    if (keys_for(action).contains(key)) {
      return action;
    }
  }
  return std::nullopt;
}

bool TeleopKeyBindings::is_pressed(Action action, const QSet<int> &pressed_keys) const {
  for (const int key : keys_for(action)) {
    if (pressed_keys.contains(key)) {
      return true;
    }
  }
  return false;
}

bool TeleopKeyBindings::is_assignable_key(int key) {
  return key != Qt::Key_unknown && key != Qt::Key_Shift && key != Qt::Key_Control &&
      key != Qt::Key_Alt && key != Qt::Key_Meta && key != Qt::Key_AltGr;
}

QList<int> TeleopKeyBindings::default_keys(Action action) {
  switch (action) {
    case kForward:
      return {Qt::Key_W, Qt::Key_Up};
    case kReverse:
      return {Qt::Key_S, Qt::Key_Down};
    case kLeft:
      return {Qt::Key_A, Qt::Key_Left};
    case kRight:
      return {Qt::Key_D, Qt::Key_Right};
    case kStop:
      return {Qt::Key_Space, Qt::Key_0};
    case kActionCount:
      return {};
  }
  return {};
}

}  // namespace car_operator_console
