#include "car_operator_console/main_window.hpp"

#include <QAction>
#include <QActionGroup>
#include <QCloseEvent>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>

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
  resize(980, 680);

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

  auto *central = new QWidget(this);
  auto *layout = new QVBoxLayout(central);

  health_group_ = new QGroupBox(central);
  auto *health_layout = new QGridLayout(health_group_);
  for (const QString name : {"agent", "lidar", "bringup", "navigation", "mapping", "patrol", "/scan", "/odom", "/imu", "/map"}) {
    const int row = health_layout->rowCount();
    auto *label = new QLabel(health_group_);
    health_layout->addWidget(new QLabel(name, health_group_), row, 0);
    health_layout->addWidget(label, row, 1);
    if (name.startsWith('/')) {
      health_labels_[name] = label;
    } else {
      component_labels_[name] = label;
    }
  }
  layout->addWidget(health_group_);

  stack_group_ = new QGroupBox(central);
  auto *stack_layout = new QHBoxLayout(stack_group_);
  auto *map_input = new QComboBox(stack_group_);
  map_input->addItems({"bot202505_map.yaml", "bot20250623_map.yaml", "fishbot_map.yaml", "test_map.yaml"});
  map_label_ = new QLabel(stack_group_);
  start_base_button_ = make_button(QString(), stack_group_);
  start_navigation_button_ = make_button(QString(), stack_group_);
  start_mapping_button_ = make_button(QString(), stack_group_);
  stop_all_button_ = make_button(QString(), stack_group_);
  stack_layout->addWidget(map_label_);
  stack_layout->addWidget(map_input);
  stack_layout->addWidget(start_base_button_);
  stack_layout->addWidget(start_navigation_button_);
  stack_layout->addWidget(start_mapping_button_);
  stack_layout->addWidget(stop_all_button_);
  layout->addWidget(stack_group_);
  connect(start_base_button_, &QPushButton::clicked, &bridge_, [this] { bridge_.start_stack("base"); });
  connect(start_navigation_button_, &QPushButton::clicked, &bridge_, [this, map_input] {
    bridge_.start_stack("navigation", map_input->currentText());
  });
  connect(start_mapping_button_, &QPushButton::clicked, &bridge_, [this] { bridge_.start_stack("mapping"); });
  connect(stop_all_button_, &QPushButton::clicked, &bridge_, [this] { bridge_.stop_stack("all"); });

  navigation_group_ = new QGroupBox(central);
  auto *navigation_layout = new QFormLayout(navigation_group_);
  x_input_ = make_coordinate_input(navigation_group_, -100.0, 100.0, 0.0);
  y_input_ = make_coordinate_input(navigation_group_, -100.0, 100.0, 0.0);
  yaw_input_ = make_coordinate_input(navigation_group_, -360.0, 360.0, 0.0);
  x_label_ = new QLabel(navigation_group_);
  y_label_ = new QLabel(navigation_group_);
  yaw_label_ = new QLabel(navigation_group_);
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
  layout->addWidget(log_, 1);
  setCentralWidget(central);

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
  retranslate_ui();
  bridge_.start();
}

MainWindow::~MainWindow() { bridge_.stop(); }

void MainWindow::closeEvent(QCloseEvent *event) {
  bridge_.stop();
  QMainWindow::closeEvent(event);
}

void MainWindow::append_log(const QString &message) { log_->appendPlainText(message); }

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
      {"Navigation (map frame, yaw in degrees)", "导航（地图坐标系，偏航角单位：度）"},
      {"Navigation-only patrol", "仅导航巡检"},
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
  map_label_->setText(ui_text("Navigation map"));
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

  for (const auto &entry : component_labels_) {
    update_component_label(entry.first);
  }
  for (const auto &entry : health_labels_) {
    update_health_label(entry.first);
  }
  update_patrol_progress_label();
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
}

void MainWindow::update_patrol_progress_label() {
  if (!has_patrol_progress_) {
    patrol_progress_->setText(ui_text("Idle"));
    return;
  }
  patrol_progress_->setText(
      ui_text("%1/%2: %3").arg(patrol_current_).arg(patrol_total_).arg(patrol_point_));
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
  }
  append_log(ui_text("%1: %2").arg(component, detail));
}

void MainWindow::update_health(const QString &topic, bool active) {
  health_activity_[topic] = active;
  update_health_label(topic);
}

}  // namespace car_operator_console
