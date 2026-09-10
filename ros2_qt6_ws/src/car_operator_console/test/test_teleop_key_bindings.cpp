#include <QCoreApplication>
#include <QSettings>
#include <QtTest/QtTest>

#include "car_operator_console/teleop_key_bindings.hpp"

class TeleopKeyBindingsTest final : public QObject {
  Q_OBJECT

 private slots:
  void initTestCase() {
    QSettings::setPath(QSettings::NativeFormat, QSettings::UserScope,
        QStringLiteral("/tmp/car_operator_console_qsettings_tests"));
  }

  void init() {
    QSettings settings(QSettings::NativeFormat, QSettings::UserScope,
        QStringLiteral("ROS 2 Car"), QStringLiteral("ROS 2 Car Console"));
    settings.remove("teleop");
  }

  void provides_default_wasd_and_arrow_keys() {
    car_operator_console::TeleopKeyBindings bindings;
    QVERIFY(bindings.keys_for(car_operator_console::TeleopKeyBindings::kForward).contains(Qt::Key_W));
    QVERIFY(bindings.keys_for(car_operator_console::TeleopKeyBindings::kForward).contains(Qt::Key_Up));
    QCOMPARE(*bindings.action_for_key(Qt::Key_Space), car_operator_console::TeleopKeyBindings::kStop);
  }

  void persists_custom_keys_and_rejects_duplicates() {
    car_operator_console::TeleopKeyBindings bindings;
    QString error;
    QVERIFY(bindings.set_custom_key(car_operator_console::TeleopKeyBindings::kForward, Qt::Key_I, &error));
    QCOMPARE(bindings.custom_key(car_operator_console::TeleopKeyBindings::kForward), static_cast<int>(Qt::Key_I));
    QVERIFY(!bindings.set_custom_key(car_operator_console::TeleopKeyBindings::kReverse, Qt::Key_I, &error));
    QCOMPARE(error, QString("That key is already assigned"));
    QSettings settings(QSettings::NativeFormat, QSettings::UserScope,
        QStringLiteral("ROS 2 Car"), QStringLiteral("ROS 2 Car Console"));
    QCOMPARE(settings.value("teleop/key/forward").toInt(), static_cast<int>(Qt::Key_I));

    car_operator_console::TeleopKeyBindings restored;
    QCOMPARE(restored.custom_key(car_operator_console::TeleopKeyBindings::kForward), static_cast<int>(Qt::Key_I));
    restored.reset();
    QCOMPARE(restored.custom_key(car_operator_console::TeleopKeyBindings::kForward), 0);
  }

  void reports_pressed_actions() {
    car_operator_console::TeleopKeyBindings bindings;
    QSet<int> pressed = {Qt::Key_W, Qt::Key_Right};
    QVERIFY(bindings.is_pressed(car_operator_console::TeleopKeyBindings::kForward, pressed));
    QVERIFY(bindings.is_pressed(car_operator_console::TeleopKeyBindings::kRight, pressed));
    QVERIFY(!bindings.is_pressed(car_operator_console::TeleopKeyBindings::kReverse, pressed));
  }
};

QTEST_APPLESS_MAIN(TeleopKeyBindingsTest)
#include "test_teleop_key_bindings.moc"
