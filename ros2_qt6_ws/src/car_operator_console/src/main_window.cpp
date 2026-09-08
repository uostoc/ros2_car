#include "car_operator_console/main_window.hpp"

#include <ament_index_cpp/get_package_share_directory.hpp>

#include <QAction>
#include <QActionGroup>
#include <QApplication>
#include <QCloseEvent>
#include <QComboBox>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRegularExpression>
#include <QScrollArea>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QWidget>

#include <rviz_common/ros_integration/ros_node_abstraction.hpp>
#include <rviz_common/visualization_frame.hpp>
#include <rviz_common/visualization_manager.hpp>


namespace car_operator_console {

namespace {

QPushButton *make_button(const QString &text, QWidget *parent) {
  auto *button = new QPushButton(text, parent);
  button->setMinimumHeight(32);
  return button;
}

QDoubleSpinBox *make_coordinate_input(QWidget *parent, double minimum, double maximum, double value) {
  auto *input = new QDoubleSpinBox(parent);
  input->setRange(minimum, maximum);
  input->setDecimals(3);
  input->setValue(value);
  return input;
}

}  // namespace

MainWindow::MainWindow(
    const QString &mode, QWidget *parent, const QString &translations_directory)
    : QMainWindow(parent),
      bridge_(this),
      language_manager_(translations_directory, this),
      mode_(mode) {
  resize(1060, 760);
  setMinimumSize(900, 620);
  setStyleSheet(R"(
    QMainWindow { background: #f5f7fb; }
    QGroupBox { background: white; border: 1px solid #dce3ef; border-radius: 10px;
                margin-top: 14px; padding: 12px; font-weight: 600; color: #24324a; }
    QGroupBox::title { subcontrol-origin: margin; left: 14px; padding: 0 5px; }
    QLabel#modeSummary { background: #eaf2ff; border: 1px solid #c8dcff; border-radius: 10px;
                          color: #174a8c; font-size: 15px; font-weight: 600; padding: 10px 14px; }
    QLabel#safetyHint { color: #6b7280; padding: 2px 4px; }
    QPushButton { min-height: 32px; border: 1px solid #b9c8dc; border-radius: 7px;
                  background: #ffffff; color: #223047; padding: 4px 11px; font-weight: 600; }
    QPushButton:hover { background: #edf4ff; border-color: #6b9de2; }
    QPushButton:disabled { background: #eef1f5; color: #9aa5b5; border-color: #d9dee7; }
    QPushButton#primaryAction { background: #2367c9; color: white; border-color: #2367c9; }
    QPushButton#primaryAction:hover { background: #1b56a7; }
    QPushButton#dangerAction { background: #c93636; color: white; border-color: #c93636; }
    QPushButton#dangerAction:hover { background: #a82424; }
    QComboBox, QDoubleSpinBox { border: 1px solid #b9c8dc; border-radius: 6px;
                                 padding: 2px 7px; background: white; }
    QComboBox { min-height: 34px; }
    QDoubleSpinBox { min-height: 40px; }
    QComboBox QAbstractItemView { border: 1px solid #b9c8dc; selection-background-color: #dceaff;
                                  selection-color: #172033; outline: 0; }
    QPlainTextEdit { background: #172033; border: 0; border-radius: 8px; color: #dbeafe;
                     font-family: monospace; padding: 8px; }
    QScrollArea { border: 0; background: #f5f7fb; }
    QScrollBar:vertical { background: #e8edf5; width: 12px; margin: 4px 2px; border-radius: 6px; }
    QScrollBar::handle:vertical { background: #9db4d1; min-height: 36px; border-radius: 5px; }
    QScrollBar::handle:vertical:hover { background: #6e91bc; }
    QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }
    QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: transparent; }
  )");

  language_menu_ = menuBar()->addMenu(QString());
  language_menu_->setObjectName("languageMenu");
  language_action_group_ = new QActionGroup(this);
  language_action_group_->setExclusive(true);
  english_action_ = language_menu_->addAction(QString());
  simplified_chinese_action_ = language_menu_->addAction(QString());
  english_action_->setObjectName("englishLanguageAction");
  simplified_chinese_action_->setObjectName("simplifiedChineseLanguageAction");
  english_action_->setCheckable(true);
  simplified_chinese_action_->setCheckable(true);
  language_action_group_->addAction(english_action_);
  language_action_group_->addAction(simplified_chinese_action_);
  connect(english_action_, &QAction::triggered, this, [this](bool checked) {
    if (checked) {
      language_manager_.set_language("en");
    }
  });
  connect(simplified_chinese_action_, &QAction::triggered, this, [this](bool checked) {
    if (checked) {
      language_manager_.set_language("zh_CN");
    }
  });

  auto *central = new QWidget();
  auto *layout = new QVBoxLayout(central);
  layout->setContentsMargins(18, 14, 18, 18);
  layout->setSpacing(12);
  layout->setSizeConstraint(QLayout::SetMinimumSize);

  mode_summary_ = new QLabel(central);
  mode_summary_->setObjectName("modeSummary");
  mode_summary_->setWordWrap(true);
  safety_hint_ = new QLabel(central);
  safety_hint_->setObjectName("safetyHint");
  safety_hint_->setWordWrap(true);
  layout->addWidget(mode_summary_);
  layout->addWidget(safety_hint_);

  health_group_ = new QGroupBox(central);
  auto *health_layout = new QGridLayout(health_group_);
  health_layout->setHorizontalSpacing(24);
  health_layout->setVerticalSpacing(5);
  health_layout->setColumnStretch(1, 1);
  health_layout->setColumnStretch(3, 1);
  const QStringList health_items = {
      "agent", "lidar", "bringup", "navigation", "mapping",
      "patrol", "/scan", "/odom", "/imu", "/map"};
  for (qsizetype index = 0; index < health_items.size(); ++index) {
    const QString &name = health_items.at(index);
    const int row = static_cast<int>(index % 5);
    const int column = index < 5 ? 0 : 2;
    auto *name_label = new QLabel(name, health_group_);
    auto *label = new QLabel(health_group_);
    name_label->setMinimumHeight(28);
    label->setMinimumHeight(28);
    label->setTextInteractionFlags(Qt::TextSelectableByMouse);
    health_layout->addWidget(name_label, row, column);
    health_layout->addWidget(label, row, column + 1);
    if (name.startsWith('/')) {
      health_labels_[name] = label;
    } else {
      component_labels_[name] = label;
    }
  }
  layout->addWidget(health_group_);

  stack_group_ = new QGroupBox(central);
  auto *stack_layout = new QVBoxLayout(stack_group_);
  auto *map_layout = new QHBoxLayout();
  auto *action_layout = new QGridLayout();
  action_layout->setHorizontalSpacing(10);
  map_input_ = new QComboBox(stack_group_);
  map_input_->setMinimumWidth(300);
  map_input_->setSizeAdjustPolicy(QComboBox::AdjustToContents);
  refresh_assets_button_ = make_button(QString(), stack_group_);
  map_label_ = new QLabel(stack_group_);
  start_base_button_ = make_button(QString(), stack_group_);
  start_navigation_button_ = make_button(QString(), stack_group_);
  start_mapping_button_ = make_button(QString(), stack_group_);
  stop_all_button_ = make_button(QString(), stack_group_);
  map_layout->addWidget(map_label_);
  map_layout->addWidget(map_input_, 1);
  map_layout->addWidget(refresh_assets_button_);
  action_layout->addWidget(start_base_button_, 0, 0);
  action_layout->addWidget(start_navigation_button_, 0, 1);
  action_layout->addWidget(start_mapping_button_, 0, 2);
  action_layout->addWidget(stop_all_button_, 0, 3);
  action_layout->setColumnStretch(0, 1);
  action_layout->setColumnStretch(1, 1);
  action_layout->setColumnStretch(2, 1);
  action_layout->setColumnStretch(3, 1);
  stack_layout->addLayout(map_layout);
  stack_layout->addLayout(action_layout);
  layout->addWidget(stack_group_);
  start_navigation_button_->setObjectName("primaryAction");
  stop_all_button_->setObjectName("dangerAction");
  connect(start_base_button_, &QPushButton::clicked, &bridge_, [this] {
    bridge_.start_stack("base");
    append_log(ui_text("Requested base stack start"));
  });
  connect(start_navigation_button_, &QPushButton::clicked, &bridge_, [this] {
    if (confirm_mode_switch(ui_text("Navigation"))) {
      bridge_.start_stack("navigation", map_input_->currentText());
    }
  });
  connect(start_mapping_button_, &QPushButton::clicked, &bridge_, [this] {
    if (confirm_mode_switch(ui_text("Mapping"))) {
      bridge_.start_stack("mapping");
    }
  });
  connect(stop_all_button_, &QPushButton::clicked, &bridge_, [this] {
    if (QMessageBox::question(this, ui_text("Stop all components?"),
        ui_text("This stops the base stack and any active navigation or mapping task."),
        QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel) == QMessageBox::Yes) {
      bridge_.stop_stack("all");
    }
  });
  connect(refresh_assets_button_, &QPushButton::clicked, this, &MainWindow::refresh_assets);

  navigation_group_ = new QGroupBox(central);
  auto *navigation_layout = new QFormLayout(navigation_group_);
  navigation_layout->setVerticalSpacing(8);
  navigation_layout->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);
  x_input_ = make_coordinate_input(navigation_group_, -100.0, 100.0, 0.0);
  y_input_ = make_coordinate_input(navigation_group_, -100.0, 100.0, 0.0);
  yaw_input_ = make_coordinate_input(navigation_group_, -360.0, 360.0, 0.0);
  x_label_ = new QLabel(navigation_group_);
  y_label_ = new QLabel(navigation_group_);
  yaw_label_ = new QLabel(navigation_group_);
  x_label_->setMinimumHeight(40);
  y_label_->setMinimumHeight(40);
  yaw_label_->setMinimumHeight(40);
  navigation_layout->addRow(x_label_, x_input_);
  navigation_layout->addRow(y_label_, y_input_);
  navigation_layout->addRow(yaw_label_, yaw_input_);
  auto *navigation_buttons = new QHBoxLayout();
  set_initial_pose_button_ = make_button(QString(), navigation_group_);
  send_goal_button_ = make_button(QString(), navigation_group_);
  cancel_goal_button_ = make_button(QString(), navigation_group_);
  navigation_buttons->addWidget(set_initial_pose_button_);
  navigation_buttons->addWidget(send_goal_button_);
  navigation_buttons->addWidget(cancel_goal_button_);
  navigation_layout->addRow(navigation_buttons);
  layout->addWidget(navigation_group_);
  connect(set_initial_pose_button_, &QPushButton::clicked, this, [this] {
    bridge_.set_initial_pose(x_input_->value(), y_input_->value(), yaw_input_->value());
  });
  connect(send_goal_button_, &QPushButton::clicked, this, [this] {
    if (!console_state_.can_navigate()) {
      append_log(ui_text("Navigation stack is not running"));
      return;
    }
    bridge_.send_navigation_goal(x_input_->value(), y_input_->value(), yaw_input_->value());
  });
  connect(cancel_goal_button_, &QPushButton::clicked, &bridge_, &RosBridge::cancel_navigation);

  patrol_group_ = new QGroupBox(central);
  auto *patrol_layout = new QHBoxLayout(patrol_group_);
  route_label_ = new QLabel(patrol_group_);
  route_input_ = new QComboBox(patrol_group_);
  route_input_->setMinimumWidth(280);
  route_input_->setSizeAdjustPolicy(QComboBox::AdjustToContents);
  route_input_->addItem("default");
  start_patrol_button_ = make_button(QString(), patrol_group_);
  cancel_patrol_button_ = make_button(QString(), patrol_group_);
  patrol_progress_ = new QLabel(patrol_group_);
  patrol_layout->addWidget(route_label_);
  patrol_layout->addWidget(route_input_);
  patrol_layout->addWidget(start_patrol_button_);
  patrol_layout->addWidget(cancel_patrol_button_);
  patrol_layout->addWidget(patrol_progress_);
  layout->addWidget(patrol_group_);
  connect(start_patrol_button_, &QPushButton::clicked, this, [this] {
    if (!console_state_.can_navigate()) {
      append_log(ui_text("Navigation stack is not running"));
      return;
    }
    bridge_.start_patrol(route_input_->currentText());
  });
  connect(cancel_patrol_button_, &QPushButton::clicked, &bridge_, &RosBridge::cancel_patrol);

  log_ = new QPlainTextEdit(central);
  log_->setReadOnly(true);
  log_->setMinimumHeight(140);
  layout->addWidget(log_, 1);
  auto *content_scroll_area = new QScrollArea(this);
  content_scroll_area->setObjectName("mainContentScrollArea");
  content_scroll_area->setWidgetResizable(true);
  content_scroll_area->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
  content_scroll_area->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  content_scroll_area->setWidget(central);

  main_tabs_ = new QTabWidget(this);
  main_tabs_->setObjectName("mainTabs");
  main_tabs_->addTab(content_scroll_area, QString());
  rviz_page_ = new QWidget(main_tabs_);
  rviz_layout_ = new QVBoxLayout(rviz_page_);
  rviz_layout_->setContentsMargins(0, 0, 0, 0);
  rviz_placeholder_ = new QLabel(rviz_page_);
  rviz_placeholder_->setObjectName("rvizPlaceholder");
  rviz_placeholder_->setAlignment(Qt::AlignCenter);
  rviz_placeholder_->setWordWrap(true);
  rviz_layout_->addWidget(rviz_placeholder_);
  main_tabs_->addTab(rviz_page_, QString());
  setCentralWidget(main_tabs_);

  connect(&language_manager_, &LanguageManager::language_changed, this,
          [this](const QString &) { retranslate_ui(); });
  connect(&bridge_, &RosBridge::component_status, this, &MainWindow::update_component);
  connect(&bridge_, &RosBridge::topic_health, this, &MainWindow::update_health);
  connect(&bridge_, &RosBridge::manager_reply, this, [this](bool success, const QString &message) {
    append_log(ui_text("Manager: %1 — %2").arg(success ? ui_text("OK") : ui_text("Error"), message));
  });
  connect(&bridge_, &RosBridge::navigation_status, this, &MainWindow::append_log);
  connect(&bridge_, &RosBridge::patrol_status, this, &MainWindow::append_log);
  connect(&bridge_, &RosBridge::patrol_progress, this,
          [this](unsigned int current, unsigned int total, const QString &point) {
            has_patrol_progress_ = true;
            patrol_current_ = current;
            patrol_total_ = total;
            patrol_point_ = point;
            update_patrol_progress_label();
          });
  connect(main_tabs_, &QTabWidget::currentChanged, this, [this](int index) {
    if (main_tabs_->widget(index) == rviz_page_) {
      initialize_rviz();
    }
  });
  retranslate_ui();
  refresh_assets();
  update_mode_summary();
  update_controls();
  bridge_.start();
}

MainWindow::~MainWindow() {
  shutdown_rviz();
  bridge_.stop();
}

void MainWindow::closeEvent(QCloseEvent *event) {
  shutdown_rviz();
  bridge_.stop();
  QMainWindow::closeEvent(event);
}

void MainWindow::append_log(const QString &message) {
  log_->appendPlainText(QString("[%1] %2").arg(
      QDateTime::currentDateTime().toString("HH:mm:ss"), message));
}

QString MainWindow::ui_text(const char *source) const {
  const QString source_text = QString::fromUtf8(source);
  const QString translated_text = tr(source);
  if (language_manager_.language() != "zh_CN" || translated_text != source_text) {
    return translated_text;
  }

  static const std::map<QString, QString> kChineseFallback = {
      {"vehicle", "车载端"},
      {"operator", "操作员端"},
      {"ROS 2 Car Console — %1", "ROS 2 小车控制台 — %1"},
      {"Language", "语言"},
      {"Simplified Chinese", "简体中文"},
      {"Vehicle health", "车辆状态"},
      {"Stack management", "系统栈管理"},
      {"Current mode: %1", "当前模式：%1"},
      {"Start Base first, then choose one high-level mode. Mapping and navigation are mutually exclusive.", "先启动基础栈，再选择一种高层模式。建图与导航不能同时运行。"},
      {"Base ready", "基础栈就绪"},
      {"Navigation", "导航"},
      {"Mapping", "建图"},
      {"No high-level mode is active", "未启动高层模式"},
      {"Refresh maps and routes", "刷新地图与路线"},
      {"Choose the saved map used for AMCL localization", "选择 AMCL 定位要使用的已保存地图"},
      {"Requested base stack start", "已请求启动基础栈"},
      {"Switch to %1?", "切换到%1？"},
      {"Starting %1 will stop the currently active high-level mode.", "启动%1会停止当前运行的高层模式。"},
      {"Stop all components?", "停止全部组件？"},
      {"This stops the base stack and any active navigation or mapping task.", "这会停止基础栈及正在运行的导航或建图任务。"},
      {"No map files found", "未找到地图文件"},
      {"No patrol routes found", "未找到巡检路线"},
      {"Navigation (map frame, yaw in degrees)", "导航（地图坐标系，偏航角单位：度）"},
      {"Navigation-only patrol", "仅导航巡检"},
      {"Console", "控制台"},
      {"Map view", "地图视图"},
      {"Open this tab to start the embedded RViz map, laser, TF, and robot view.", "打开此标签页以启动内嵌 RViz 地图、激光、TF 和机器人模型视图。"},
      {"Embedded RViz", "内嵌 RViz"},
      {"Embedded RViz started", "内嵌 RViz 已启动"},
      {"Embedded RViz could not start", "内嵌 RViz 无法启动"},
      {"Embedded RViz could not start: %1", "内嵌 RViz 无法启动：%1"},
      {"Navigation map", "导航地图"},
      {"Yaw", "偏航角"},
      {"Route", "路线"},
      {"Start Base", "启动基础栈"},
      {"Start Navigation", "启动导航"},
      {"Start Mapping", "启动建图"},
      {"Stop All", "停止全部"},
      {"Set Initial Pose", "设置初始位姿"},
      {"Send Goal", "发送目标"},
      {"Cancel Goal", "取消目标"},
      {"Start Patrol", "开始巡检"},
      {"Cancel Patrol", "取消巡检"},
      {"Unknown", "未知"},
      {"Active", "活跃"},
      {"Inactive", "未活跃"},
      {"Idle", "空闲"},
      {"Stopped", "已停止"},
      {"Starting", "启动中"},
      {"Running", "运行中"},
      {"Stopping", "停止中"},
      {"Error", "错误"},
      {"OK", "成功"},
      {"Navigation stack is not running", "导航栈尚未运行"},
      {"Manager: %1 — %2", "管理服务：%1 — %2"},
      {"%1 (PID %2): %3", "%1（进程号 %2）：%3"},
      {"%1/%2: %3", "%1/%2：%3"},
      {"%1: %2", "%1：%2"},
  };
  const auto fallback = kChineseFallback.find(source_text);
  return fallback == kChineseFallback.end() ? source_text : fallback->second;
}

void MainWindow::retranslate_ui() {
  QString mode_text = mode_;
  if (mode_ == "vehicle") {
    mode_text = ui_text("vehicle");
  } else if (mode_ == "operator") {
    mode_text = ui_text("operator");
  }
  setWindowTitle(ui_text("ROS 2 Car Console — %1").arg(mode_text));
  language_menu_->setTitle(ui_text("Language"));
  english_action_->setText(ui_text("English"));
  simplified_chinese_action_->setText(ui_text("Simplified Chinese"));
  english_action_->setChecked(language_manager_.language() == "en");
  simplified_chinese_action_->setChecked(language_manager_.language() == "zh_CN");

  health_group_->setTitle(ui_text("Vehicle health"));
  stack_group_->setTitle(ui_text("Stack management"));
  navigation_group_->setTitle(ui_text("Navigation (map frame, yaw in degrees)"));
  patrol_group_->setTitle(ui_text("Navigation-only patrol"));
  main_tabs_->setTabText(main_tabs_->indexOf(main_tabs_->widget(0)), ui_text("Console"));
  main_tabs_->setTabText(main_tabs_->indexOf(rviz_page_), ui_text("Map view"));
  if (rviz_placeholder_ != nullptr) {
    rviz_placeholder_->setText(ui_text(
        "Open this tab to start the embedded RViz map, laser, TF, and robot view."));
  }
  map_label_->setText(ui_text("Navigation map"));
  map_input_->setToolTip(ui_text("Choose the saved map used for AMCL localization"));
  refresh_assets_button_->setText(ui_text("Refresh maps and routes"));
  x_label_->setText(ui_text("X"));
  y_label_->setText(ui_text("Y"));
  yaw_label_->setText(ui_text("Yaw"));
  route_label_->setText(ui_text("Route"));
  start_base_button_->setText(ui_text("Start Base"));
  start_navigation_button_->setText(ui_text("Start Navigation"));
  start_mapping_button_->setText(ui_text("Start Mapping"));
  stop_all_button_->setText(ui_text("Stop All"));
  set_initial_pose_button_->setText(ui_text("Set Initial Pose"));
  send_goal_button_->setText(ui_text("Send Goal"));
  cancel_goal_button_->setText(ui_text("Cancel Goal"));
  start_patrol_button_->setText(ui_text("Start Patrol"));
  cancel_patrol_button_->setText(ui_text("Cancel Patrol"));

  safety_hint_->setText(ui_text(
      "Start Base first, then choose one high-level mode. Mapping and navigation are mutually exclusive."));
  update_mode_summary();

  for (const auto &entry : component_labels_) {
    update_component_label(entry.first);
  }
  for (const auto &entry : health_labels_) {
    update_health_label(entry.first);
  }
  update_patrol_progress_label();
}

void MainWindow::initialize_rviz() {
  if (rviz_initialization_attempted_) {
    return;
  }
  rviz_initialization_attempted_ = true;

  try {
    const QString config_path = QString::fromStdString(
        ament_index_cpp::get_package_share_directory("car_operator_console")) +
        "/config/embedded_view.rviz";
    rviz_node_ = std::make_shared<rviz_common::ros_integration::RosNodeAbstraction>(
        "car_operator_console_rviz");
    rviz_frame_ = new rviz_common::VisualizationFrame(rviz_node_, rviz_page_);
    // VisualizationFrame defaults to a top-level QMainWindow.  Re-parenting it with
    // Qt::Widget before initialization makes it a permanent child of the map tab.
    rviz_frame_->setParent(rviz_page_, Qt::Widget);
    rviz_frame_->setWindowFlags(Qt::Widget);
    rviz_frame_->setApp(qobject_cast<QApplication *>(QCoreApplication::instance()));
    rviz_frame_->setSplashPath(QString());
    rviz_frame_->initialize(rviz_node_, config_path);

    rviz_layout_->removeWidget(rviz_placeholder_);
    rviz_placeholder_->deleteLater();
    rviz_placeholder_ = nullptr;
    rviz_layout_->addWidget(rviz_frame_);
    append_log(ui_text("Embedded RViz started"));
  } catch (const std::exception &error) {
    shutdown_rviz();
    rviz_placeholder_->setText(ui_text("Embedded RViz could not start: %1").arg(error.what()));
    append_log(ui_text("Embedded RViz could not start: %1").arg(error.what()));
  } catch (...) {
    shutdown_rviz();
    rviz_placeholder_->setText(ui_text("Embedded RViz could not start"));
    append_log(ui_text("Embedded RViz could not start"));
  }
}

void MainWindow::shutdown_rviz() {
  if (rviz_frame_ != nullptr) {
    if (rviz_frame_->getManager() != nullptr) {
      rviz_frame_->getManager()->stopUpdate();
    }
    delete rviz_frame_;
    rviz_frame_ = nullptr;
  }
  rviz_node_.reset();
}

void MainWindow::update_component_label(const QString &component) {
  const auto label = component_labels_.find(component);
  if (label == component_labels_.end()) {
    return;
  }
  const auto display = component_display_.find(component);
  if (display == component_display_.end()) {
    label->second->setText(ui_text("Unknown"));
    return;
  }
  label->second->setText(
      ui_text("%1 (PID %2): %3").arg(state_text(display->second.state)).arg(display->second.pid).arg(display->second.detail));
  const QString color = display->second.state == ConsoleState::kRunning ? "#197a4a" :
      display->second.state == ConsoleState::kError ? "#b42318" : "#526071";
  label->second->setStyleSheet(QString("color: %1; font-weight: 600;").arg(color));
}

void MainWindow::update_health_label(const QString &topic) {
  const auto label = health_labels_.find(topic);
  if (label == health_labels_.end()) {
    return;
  }
  const auto activity = health_activity_.find(topic);
  if (activity == health_activity_.end()) {
    label->second->setText(ui_text("Unknown"));
    return;
  }
  label->second->setText(activity->second ? ui_text("Active") : ui_text("Inactive"));
  label->second->setStyleSheet(activity->second ? "color: #197a4a; font-weight: 600;" :
      "color: #8a4b08; font-weight: 600;");
}

void MainWindow::update_patrol_progress_label() {
  if (!has_patrol_progress_) {
    patrol_progress_->setText(ui_text("Idle"));
    return;
  }
  patrol_progress_->setText(
      ui_text("%1/%2: %3").arg(patrol_current_).arg(patrol_total_).arg(patrol_point_));
}

void MainWindow::update_mode_summary() {
  QString mode = ui_text("No high-level mode is active");
  if (console_state_.can_navigate()) {
    mode = ui_text("Navigation");
  } else if (console_state_.is_mapping()) {
    mode = ui_text("Mapping");
  }
  mode_summary_->setText(ui_text("Current mode: %1").arg(mode));
}

void MainWindow::update_controls() {
  const bool navigation_ready = console_state_.can_navigate();
  start_navigation_button_->setEnabled(console_state_.can_start_navigation() && map_input_->count() > 0);
  start_mapping_button_->setEnabled(console_state_.can_start_mapping());
  set_initial_pose_button_->setEnabled(navigation_ready);
  send_goal_button_->setEnabled(navigation_ready);
  start_patrol_button_->setEnabled(navigation_ready && route_input_->count() > 0);
}

void MainWindow::refresh_assets() {
  const QString current_map = map_input_->currentText();
  const QString current_route = route_input_->currentText();
  map_input_->clear();
  route_input_->clear();

  try {
    const QString map_directory = QString::fromStdString(
        ament_index_cpp::get_package_share_directory("fishbot_navigation2")) + "/maps";
    const QDir directory(map_directory);
    const QFileInfoList maps = directory.entryInfoList({"*.yaml"}, QDir::Files, QDir::Name);
    for (const QFileInfo &map : maps) {
      map_input_->addItem(map.fileName());
    }
  } catch (const std::exception &) {
    append_log(ui_text("No map files found"));
  }

  try {
    const QString routes_path = QString::fromStdString(
        ament_index_cpp::get_package_share_directory("car_patrol")) + "/config/patrol_routes.yaml";
    QFile routes_file(routes_path);
    if (routes_file.open(QIODevice::ReadOnly | QIODevice::Text)) {
      const QRegularExpression route_pattern("^ {2}([A-Za-z0-9_-]+):\\s*$");
      while (!routes_file.atEnd()) {
        const QString line = QString::fromUtf8(routes_file.readLine());
        const QRegularExpressionMatch match = route_pattern.match(line);
        if (match.hasMatch()) {
          route_input_->addItem(match.captured(1));
        }
      }
    }
  } catch (const std::exception &) {
    append_log(ui_text("No patrol routes found"));
  }

  const int map_index = map_input_->findText(current_map);
  map_input_->setCurrentIndex(map_index >= 0 ? map_index : 0);
  const int route_index = route_input_->findText(current_route);
  route_input_->setCurrentIndex(route_index >= 0 ? route_index : 0);
  update_controls();
}

bool MainWindow::confirm_mode_switch(const QString &target_mode) {
  const bool switching = (target_mode == ui_text("Navigation") && console_state_.is_mapping()) ||
      (target_mode == ui_text("Mapping") && console_state_.can_navigate());
  if (!switching) {
    return true;
  }
  return QMessageBox::question(this, ui_text("Switch to %1?").arg(target_mode),
      ui_text("Starting %1 will stop the currently active high-level mode.").arg(target_mode),
      QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel) == QMessageBox::Yes;
}

QString MainWindow::state_text(int state) const {
  switch (state) {
    case ConsoleState::kStopped:
      return ui_text("Stopped");
    case ConsoleState::kStarting:
      return ui_text("Starting");
    case ConsoleState::kRunning:
      return ui_text("Running");
    case ConsoleState::kStopping:
      return ui_text("Stopping");
    case ConsoleState::kError:
      return ui_text("Error");
    default:
      return ui_text("Unknown");
  }
}

void MainWindow::update_component(const QString &component, int state, int pid, const QString &detail) {
  component_display_[component] = {state, pid, detail};
  update_component_label(component);
  const auto health_iterator = health_labels_.find(component);
  if (health_iterator != health_labels_.end()) {
    health_activity_[component] = state == ConsoleState::kRunning;
    update_health_label(component);
  }
  if (component == "navigation") {
    console_state_.navigation_state = state;
  } else if (component == "mapping") {
    console_state_.mapping_state = state;
  }
  update_mode_summary();
  update_controls();
  append_log(ui_text("%1: %2").arg(component, detail));
}

void MainWindow::update_health(const QString &topic, bool active) {
  health_activity_[topic] = active;
  update_health_label(topic);
}

}  // namespace car_operator_console
