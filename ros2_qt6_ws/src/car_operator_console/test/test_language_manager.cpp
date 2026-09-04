#include <QAction>
#include <QCoreApplication>
#include <QMenu>
#include <QSettings>
#include <QSignalSpy>
#include <QtTest/QtTest>

#include <rclcpp/rclcpp.hpp>

#include "car_operator_console/language_manager.hpp"
#include "car_operator_console/main_window.hpp"

namespace {

QString translations_directory() {
  return QStringLiteral(CAR_OPERATOR_CONSOLE_TEST_TRANSLATIONS_DIR);
}

}  // namespace

class LanguageManagerTest final : public QObject {
  Q_OBJECT

 private slots:
  void initTestCase() {
    QCoreApplication::setOrganizationName("ROS 2 Car Console Tests");
    QCoreApplication::setApplicationName("Language Manager Tests");
    int argc = 0;
    char **argv = nullptr;
    rclcpp::init(argc, argv);
  }

  void cleanup() {
    QSettings settings;
    settings.setValue("ui/language", "en");
  }

  void cleanupTestCase() { rclcpp::shutdown(); }

  void defaults_to_english_without_saved_value() {
    QSettings settings;
    settings.remove("ui/language");

    car_operator_console::LanguageManager manager(translations_directory());

    QCOMPARE(manager.language(), QString("en"));
    QCOMPARE(QCoreApplication::translate("MainWindow", "Vehicle health"), QString("Vehicle health"));
  }

  void switches_to_chinese_and_persists_the_selection() {
    {
      car_operator_console::LanguageManager manager(translations_directory());
      QSignalSpy signal_spy(&manager, &car_operator_console::LanguageManager::language_changed);

      QVERIFY(manager.set_language("zh_CN"));
      QCOMPARE(signal_spy.count(), 1);
      QCOMPARE(manager.language(), QString("zh_CN"));
      QCOMPARE(QCoreApplication::translate("MainWindow", "Vehicle health"), QString("车辆状态"));
    }

    car_operator_console::LanguageManager restored_manager(translations_directory());
    QCOMPARE(restored_manager.language(), QString("zh_CN"));
    QCOMPARE(QCoreApplication::translate("MainWindow", "Start Navigation"), QString("启动导航"));
  }

  void preserves_chinese_selection_when_a_catalog_is_unavailable() {
    car_operator_console::LanguageManager manager("translations-that-do-not-exist");

    QVERIFY(!manager.set_language("zh_CN"));
    QCOMPARE(manager.language(), QString("zh_CN"));
  }

  void language_menu_retranslates_the_window_immediately() {
    QSettings settings;
    settings.setValue("ui/language", "en");
    car_operator_console::MainWindow window(
        "operator", nullptr, "translations-that-do-not-exist");
    auto *menu = window.findChild<QMenu *>("languageMenu");
    auto *english = window.findChild<QAction *>("englishLanguageAction");
    auto *chinese = window.findChild<QAction *>("simplifiedChineseLanguageAction");

    QVERIFY(menu != nullptr);
    QVERIFY(english != nullptr);
    QVERIFY(chinese != nullptr);
    QCOMPARE(menu->title(), QString("Language"));
    QCOMPARE(window.windowTitle(), QString("ROS 2 Car Console — operator"));

    chinese->trigger();
    QCoreApplication::processEvents();

    QCOMPARE(menu->title(), QString("语言"));
    QCOMPARE(chinese->text(), QString("简体中文"));
    QVERIFY(chinese->isChecked());
    QVERIFY(!english->isChecked());
    QCOMPARE(window.windowTitle(), QString("ROS 2 小车控制台 — 操作员端"));
  }
};

QTEST_MAIN(LanguageManagerTest)
#include "test_language_manager.moc"
