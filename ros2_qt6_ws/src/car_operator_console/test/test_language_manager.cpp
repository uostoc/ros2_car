#include <QAction>
#include <QCoreApplication>
#include <QLabel>
#include <QMenu>
#include <QScrollArea>
#include <QScrollBar>
#include <QSettings>
#include <QSignalSpy>
#include <QTabWidget>
#include <QPushButton>
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

  void main_content_scrolls_vertically_only_when_needed() {
    QSettings settings;
    settings.setValue("ui/language", "en");
    car_operator_console::MainWindow window(
        "operator", nullptr, "translations-that-do-not-exist");
    auto *scroll_area = window.findChild<QScrollArea *>("mainContentScrollArea");

    QVERIFY(scroll_area != nullptr);
    QCOMPARE(scroll_area->verticalScrollBarPolicy(), Qt::ScrollBarAsNeeded);
    QCOMPARE(scroll_area->horizontalScrollBarPolicy(), Qt::ScrollBarAlwaysOff);

    window.resize(900, 620);
    window.show();
    QTest::qWait(20);
    QVERIFY(scroll_area->verticalScrollBar()->maximum() > 0);

    window.resize(1400, 1600);
    QTest::qWait(20);
    QCOMPARE(scroll_area->verticalScrollBar()->maximum(), 0);
  }

  void mapping_and_map_view_are_available_without_initializing_rviz() {
    QSettings settings;
    settings.setValue("ui/language", "en");
    car_operator_console::MainWindow window(
        "operator", nullptr, "translations-that-do-not-exist");
    auto *tabs = window.findChild<QTabWidget *>("mainTabs");

    QVERIFY(tabs != nullptr);
    QCOMPARE(tabs->count(), 3);
    QCOMPARE(tabs->tabText(0), QString("Console"));
    QCOMPARE(tabs->tabText(1), QString("Mapping"));
    QCOMPARE(tabs->tabText(2), QString("Map view"));
    QVERIFY(window.findChild<QWidget *>("mappingPage") != nullptr);
    QVERIFY(window.findChild<QPushButton *>("mappingStartButton") != nullptr);
    QVERIFY(window.findChild<QPushButton *>("mappingSaveButton") != nullptr);
    QVERIFY(window.findChild<QPushButton *>("mappingStopButton") != nullptr);
    auto *forward = window.findChild<QPushButton *>("teleopForwardButton");
    auto *reverse = window.findChild<QPushButton *>("teleopBackButton");
    auto *left = window.findChild<QPushButton *>("teleopLeftButton");
    auto *right = window.findChild<QPushButton *>("teleopRightButton");
    auto *stop = window.findChild<QPushButton *>("teleopStopButton");
    QVERIFY(forward != nullptr);
    QVERIFY(reverse != nullptr);
    QVERIFY(left != nullptr);
    QVERIFY(right != nullptr);
    QVERIFY(stop != nullptr);
    QVERIFY(!forward->isEnabled());
    QVERIFY(!reverse->isEnabled());
    QVERIFY(!left->isEnabled());
    QVERIFY(!right->isEnabled());
    QVERIFY(window.findChild<QLabel *>("rvizPlaceholder") != nullptr);
  }
};

QTEST_MAIN(LanguageManagerTest)
#include "test_language_manager.moc"
