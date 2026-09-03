#include "car_operator_console/main_window.hpp"

#include <QCloseEvent>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
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

MainWindow::MainWindow(const QString &mode, QWidget *parent) : QMainWindow(parent), bridge_(this) {
  setWindowTitle(QString("ROS 2 Car Console — %1").arg(mode));
  resize(980, 680);

  auto *central = new QWidget(this);
  auto *layout = new QVBoxLayout(central);

  auto *health_group = new QGroupBox("Vehicle health", central);
  auto *health_layout = new QGridLayout(health_group);
  for (const QString name : {"agent", "lidar", "bringup", "navigation", "mapping", "patrol", "/scan", "/odom", "/imu", "/map"}) {
    auto *label = new QLabel("Unknown", health_group);
    health_layout->addWidget(new QLabel(name, health_group), health_layout->rowCount(), 0);
    health_layout->addWidget(label, health_layout->rowCount() - 1, 1);
    if (name.startsWith('/')) {
      health_labels_[name] = label;
    } else {
      component_labels_[name] = label;
    }
  }
  layout->addWidget(health_group);

  auto *stack_group = new QGroupBox("Stack management", central);
  auto *stack_layout = new QHBoxLayout(stack_group);
  auto *map_input = new QComboBox(stack_group);
  map_input->addItems({"bot202505_map.yaml", "bot20250623_map.yaml", "fishbot_map.yaml", "test_map.yaml"});
  auto *start_base = make_button("Start Base", stack_group);
  auto *start_navigation = make_button("Start Navigation", stack_group);
  auto *start_mapping = make_button("Start Mapping", stack_group);
  auto *stop_all = make_button("Stop All", stack_group);
  stack_layout->addWidget(new QLabel("Navigation map", stack_group));
  stack_layout->addWidget(map_input);
  stack_layout->addWidget(start_base);
  stack_layout->addWidget(start_navigation);
  stack_layout->addWidget(start_mapping);
  stack_layout->addWidget(stop_all);
  layout->addWidget(stack_group);
  connect(start_base, &QPushButton::clicked, &bridge_, [this] { bridge_.start_stack("base"); });
  connect(start_navigation, &QPushButton::clicked, &bridge_, [this, map_input] {
    bridge_.start_stack("navigation", map_input->currentText());
  });
  connect(start_mapping, &QPushButton::clicked, &bridge_, [this] { bridge_.start_stack("mapping"); });
  connect(stop_all, &QPushButton::clicked, &bridge_, [this] { bridge_.stop_stack("all"); });

  auto *navigation_group = new QGroupBox("Navigation (map frame, yaw in degrees)", central);
  auto *navigation_layout = new QFormLayout(navigation_group);
  x_input_ = make_coordinate_input(navigation_group, -100.0, 100.0, 0.0);
  y_input_ = make_coordinate_input(navigation_group, -100.0, 100.0, 0.0);
  yaw_input_ = make_coordinate_input(navigation_group, -360.0, 360.0, 0.0);
  navigation_layout->addRow("X", x_input_);
  navigation_layout->addRow("Y", y_input_);
  navigation_layout->addRow("Yaw", yaw_input_);
  auto *navigation_buttons = new QHBoxLayout();
  auto *set_initial = make_button("Set Initial Pose", navigation_group);
  auto *send_goal = make_button("Send Goal", navigation_group);
  auto *cancel_goal = make_button("Cancel Goal", navigation_group);
  navigation_buttons->addWidget(set_initial);
  navigation_buttons->addWidget(send_goal);
  navigation_buttons->addWidget(cancel_goal);
  navigation_layout->addRow(navigation_buttons);
  layout->addWidget(navigation_group);
  connect(set_initial, &QPushButton::clicked, this, [this] {
    bridge_.set_initial_pose(x_input_->value(), y_input_->value(), yaw_input_->value());
  });
  connect(send_goal, &QPushButton::clicked, this, [this] {
    if (!console_state_.can_navigate()) {
      append_log("Navigation stack is not running");
      return;
    }
    bridge_.send_navigation_goal(x_input_->value(), y_input_->value(), yaw_input_->value());
  });
  connect(cancel_goal, &QPushButton::clicked, &bridge_, &RosBridge::cancel_navigation);

  auto *patrol_group = new QGroupBox("Navigation-only patrol", central);
  auto *patrol_layout = new QHBoxLayout(patrol_group);
  route_input_ = new QComboBox(patrol_group);
  route_input_->addItem("default");
  auto *start_patrol = make_button("Start Patrol", patrol_group);
  auto *cancel_patrol = make_button("Cancel Patrol", patrol_group);
  patrol_progress_ = new QLabel("Idle", patrol_group);
  patrol_layout->addWidget(route_input_);
  patrol_layout->addWidget(start_patrol);
  patrol_layout->addWidget(cancel_patrol);
  patrol_layout->addWidget(patrol_progress_);
  layout->addWidget(patrol_group);
  connect(start_patrol, &QPushButton::clicked, this, [this] {
    if (!console_state_.can_navigate()) {
      append_log("Navigation stack is not running");
      return;
    }
    bridge_.start_patrol(route_input_->currentText());
  });
  connect(cancel_patrol, &QPushButton::clicked, &bridge_, &RosBridge::cancel_patrol);

  log_ = new QPlainTextEdit(central);
  log_->setReadOnly(true);
  layout->addWidget(log_, 1);
  setCentralWidget(central);

  connect(&bridge_, &RosBridge::component_status, this, &MainWindow::update_component);
  connect(&bridge_, &RosBridge::topic_health, this, &MainWindow::update_health);
  connect(&bridge_, &RosBridge::manager_reply, this, [this](bool success, const QString &message) {
    append_log(QString("Manager: %1 — %2").arg(success ? "ok" : "error", message));
  });
  connect(&bridge_, &RosBridge::navigation_status, this, &MainWindow::append_log);
  connect(&bridge_, &RosBridge::patrol_status, this, &MainWindow::append_log);
  connect(&bridge_, &RosBridge::patrol_progress, this,
          [this](unsigned int current, unsigned int total, const QString &point) {
            patrol_progress_->setText(QString("%1/%2: %3").arg(current).arg(total).arg(point));
          });
  bridge_.start();
}

MainWindow::~MainWindow() { bridge_.stop(); }

void MainWindow::closeEvent(QCloseEvent *event) {
  bridge_.stop();
  QMainWindow::closeEvent(event);
}

void MainWindow::append_log(const QString &message) { log_->appendPlainText(message); }

QString MainWindow::state_text(int state) {
  switch (state) {
    case ConsoleState::kStopped:
      return "Stopped";
    case ConsoleState::kStarting:
      return "Starting";
    case ConsoleState::kRunning:
      return "Running";
    case ConsoleState::kStopping:
      return "Stopping";
    case ConsoleState::kError:
      return "Error";
    default:
      return "Unknown";
  }
}

void MainWindow::update_component(const QString &component, int state, int pid, const QString &detail) {
  const auto iterator = component_labels_.find(component);
  if (iterator != component_labels_.end()) {
    iterator->second->setText(QString("%1 (pid %2): %3").arg(state_text(state)).arg(pid).arg(detail));
  }
  const auto health_iterator = health_labels_.find(component);
  if (health_iterator != health_labels_.end()) {
    health_iterator->second->setText(state == ConsoleState::kRunning ? "Active" : state_text(state));
  }
  if (component == "navigation") {
    console_state_.navigation_state = state;
  }
  append_log(QString("%1: %2").arg(component, detail));
}

void MainWindow::update_health(const QString &topic, bool active) {
  const auto iterator = health_labels_.find(topic);
  if (iterator != health_labels_.end()) {
    iterator->second->setText(active ? "Active" : "Inactive");
  }
}

}  // namespace car_operator_console
