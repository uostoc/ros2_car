#include "car_operator_console/main_window.hpp"

#include <ament_index_cpp/get_package_share_directory.hpp>

#include <QAction>
#include <QActionGroup>
#include <QAbstractSpinBox>
#include <QApplication>
#include <QCheckBox>
#include <QCloseEvent>
#include <QComboBox>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QFocusEvent>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHideEvent>
#include <QKeyEvent>
#include <QKeySequence>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QProcess>
#include <QPushButton>
#include <QRegularExpression>
#include <QScrollArea>
#include <QSignalBlocker>
#include <QTabWidget>
#include <QTimer>
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
    QPushButton#primaryAction, QPushButton#mappingStartButton { background: #2367c9; color: white; border-color: #2367c9; }
    QPushButton#primaryAction:hover, QPushButton#mappingStartButton:hover { background: #1b56a7; }
    QPushButton#dangerAction, QPushButton#teleopStopButton { background: #c93636; color: white; border-color: #c93636; }
    QPushButton#dangerAction:hover, QPushButton#teleopStopButton:hover { background: #a82424; }
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
  stop_all_button_ = make_button(QString(), stack_group_);
  map_layout->addWidget(map_label_);
  map_layout->addWidget(map_input_, 1);
  map_layout->addWidget(refresh_assets_button_);
  action_layout->addWidget(start_base_button_, 0, 0);
  action_layout->addWidget(start_navigation_button_, 0, 1);
  action_layout->addWidget(stop_all_button_, 0, 2);
  action_layout->setColumnStretch(0, 1);
  action_layout->setColumnStretch(1, 1);
  action_layout->setColumnStretch(2, 1);
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
      set_teleop_enabled(false);
      bridge_.start_stack("navigation", map_input_->currentText());
    }
  });
  connect(stop_all_button_, &QPushButton::clicked, &bridge_, [this] {
    if (QMessageBox::question(this, ui_text("Stop all components?"),
        ui_text("This stops the base stack and any active navigation or mapping task."),
        QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel) == QMessageBox::Yes) {
      stop_local_mapping(true);
      bridge_.stop_stack("all");
    }
  });
  connect(refresh_assets_button_, &QPushButton::clicked, this, [this] { refresh_assets(); });

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

  mapping_page_ = new QWidget(main_tabs_);
  mapping_page_->setObjectName("mappingPage");
  auto *mapping_layout = new QVBoxLayout(mapping_page_);
  mapping_layout->setContentsMargins(18, 14, 18, 18);
  mapping_layout->setSpacing(12);
  mapping_hint_ = new QLabel(mapping_page_);
  mapping_hint_->setObjectName("mappingHint");
  mapping_hint_->setWordWrap(true);
  mapping_layout->addWidget(mapping_hint_);

  auto *mapping_health_group = new QGroupBox(mapping_page_);
  mapping_health_group->setObjectName("mappingHealthGroup");
  auto *mapping_health_layout = new QGridLayout(mapping_health_group);
  const QStringList mapping_topics = {"/scan", "/odom", "/imu", "/tf", "/map"};
  for (int index = 0; index < mapping_topics.size(); ++index) {
    const QString &topic = mapping_topics.at(index);
    auto *topic_label = new QLabel(topic, mapping_health_group);
    auto *state_label = new QLabel(mapping_health_group);
    state_label->setMinimumHeight(28);
    mapping_health_layout->addWidget(topic_label, index, 0);
    mapping_health_layout->addWidget(state_label, index, 1);
    mapping_health_layout->setColumnStretch(1, 1);
    mapping_health_labels_[topic] = state_label;
  }
  mapping_layout->addWidget(mapping_health_group);

  auto *mapping_controls_group = new QGroupBox(mapping_page_);
  mapping_controls_group->setObjectName("mappingControlsGroup");
  auto *mapping_controls_layout = new QFormLayout(mapping_controls_group);
  mapping_name_label_ = new QLabel(mapping_controls_group);
  mapping_name_input_ = new QLineEdit(mapping_controls_group);
  mapping_name_input_->setObjectName("mappingNameInput");
  mapping_name_input_->setPlaceholderText("office_20260909");
  mapping_controls_layout->addRow(mapping_name_label_, mapping_name_input_);
  mapping_state_label_ = new QLabel(mapping_controls_group);
  mapping_state_label_->setObjectName("mappingStateLabel");
  mapping_state_label_->setWordWrap(true);
  mapping_controls_layout->addRow(mapping_state_label_);
  auto *mapping_actions = new QHBoxLayout();
  mapping_start_button_ = make_button(QString(), mapping_controls_group);
  mapping_start_button_->setObjectName("mappingStartButton");
  mapping_save_button_ = make_button(QString(), mapping_controls_group);
  mapping_save_button_->setObjectName("mappingSaveButton");
  mapping_stop_button_ = make_button(QString(), mapping_controls_group);
  mapping_stop_button_->setObjectName("mappingStopButton");
  mapping_actions->addWidget(mapping_start_button_);
  mapping_actions->addWidget(mapping_save_button_);
  mapping_actions->addWidget(mapping_stop_button_);
  mapping_controls_layout->addRow(mapping_actions);
  mapping_layout->addWidget(mapping_controls_group);

  mapping_log_ = new QPlainTextEdit(mapping_page_);
  mapping_log_->setObjectName("mappingLog");
  mapping_log_->setReadOnly(true);
  mapping_log_->setMinimumHeight(220);
  mapping_layout->addWidget(mapping_log_, 1);
  main_tabs_->addTab(mapping_page_, QString());
  connect(mapping_start_button_, &QPushButton::clicked, this, &MainWindow::start_local_mapping);
  connect(mapping_save_button_, &QPushButton::clicked, this, &MainWindow::save_current_map);
  connect(mapping_stop_button_, &QPushButton::clicked, this, &MainWindow::request_stop_local_mapping);

  teleop_page_ = new QWidget(main_tabs_);
  teleop_page_->setObjectName("teleopPage");
  auto *teleop_layout = new QVBoxLayout(teleop_page_);
  teleop_layout->setContentsMargins(18, 14, 18, 18);
  teleop_layout->setSpacing(12);
  teleop_hint_ = new QLabel(teleop_page_);
  teleop_hint_->setObjectName("teleopHint");
  teleop_hint_->setWordWrap(true);
  teleop_layout->addWidget(teleop_hint_);

  auto *teleop_enable_group = new QGroupBox(teleop_page_);
  teleop_enable_group->setObjectName("teleopEnableGroup");
  auto *teleop_enable_layout = new QVBoxLayout(teleop_enable_group);
  teleop_enable_check_ = new QCheckBox(teleop_enable_group);
  teleop_enable_check_->setObjectName("teleopEnableCheck");
  teleop_status_label_ = new QLabel(teleop_enable_group);
  teleop_status_label_->setObjectName("teleopStatusLabel");
  teleop_status_label_->setWordWrap(true);
  teleop_enable_layout->addWidget(teleop_enable_check_);
  teleop_enable_layout->addWidget(teleop_status_label_);
  teleop_layout->addWidget(teleop_enable_group);

  auto *teleop_controls_group = new QGroupBox(teleop_page_);
  teleop_controls_group->setObjectName("teleopControlsGroup");
  auto *teleop_controls_layout = new QGridLayout(teleop_controls_group);
  teleop_linear_label_ = new QLabel(teleop_controls_group);
  teleop_angular_label_ = new QLabel(teleop_controls_group);
  teleop_linear_input_ = make_coordinate_input(teleop_controls_group, 0.05, 0.50, 0.15);
  teleop_angular_input_ = make_coordinate_input(teleop_controls_group, 0.10, 2.00, 0.60);
  teleop_linear_input_->setSingleStep(0.05);
  teleop_angular_input_->setSingleStep(0.10);
  teleop_controls_layout->addWidget(teleop_linear_label_, 0, 0);
  teleop_controls_layout->addWidget(teleop_linear_input_, 0, 1);
  teleop_controls_layout->addWidget(teleop_angular_label_, 0, 2);
  teleop_controls_layout->addWidget(teleop_angular_input_, 0, 3);
  const std::array<QString, TeleopKeyBindings::kActionCount> teleop_button_names = {
      "teleopForwardButton", "teleopBackButton", "teleopLeftButton", "teleopRightButton", "teleopStopButton"};
  for (int index = 0; index < TeleopKeyBindings::kActionCount; ++index) {
    auto *button = make_button(QString(), teleop_controls_group);
    button->setObjectName(teleop_button_names.at(index));
    teleop_action_buttons_.at(index) = button;
  }
  teleop_controls_layout->addWidget(teleop_action_buttons_.at(TeleopKeyBindings::kForward), 1, 1);
  teleop_controls_layout->addWidget(teleop_action_buttons_.at(TeleopKeyBindings::kLeft), 2, 0);
  teleop_controls_layout->addWidget(teleop_action_buttons_.at(TeleopKeyBindings::kStop), 2, 1);
  teleop_controls_layout->addWidget(teleop_action_buttons_.at(TeleopKeyBindings::kRight), 2, 2);
  teleop_controls_layout->addWidget(teleop_action_buttons_.at(TeleopKeyBindings::kReverse), 3, 1);
  teleop_action_buttons_.at(TeleopKeyBindings::kStop)->setObjectName("teleopStopButton");
  teleop_layout->addWidget(teleop_controls_group);

  auto *key_bindings_group = new QGroupBox(teleop_page_);
  key_bindings_group->setObjectName("teleopKeyBindingsGroup");
  auto *key_bindings_layout = new QGridLayout(key_bindings_group);
  for (int index = 0; index < TeleopKeyBindings::kActionCount; ++index) {
    auto *button = make_button(QString(), key_bindings_group);
    button->setObjectName(QString("teleopKeyCapture%1").arg(index));
    key_capture_buttons_.at(index) = button;
    connect(button, &QPushButton::clicked, this, [this, index] {
      begin_key_capture(static_cast<TeleopKeyBindings::Action>(index));
    });
    key_bindings_layout->addWidget(button, index / 2, index % 2);
  }
  teleop_reset_keys_button_ = make_button(QString(), key_bindings_group);
  teleop_reset_keys_button_->setObjectName("teleopResetKeysButton");
  key_bindings_layout->addWidget(teleop_reset_keys_button_, 3, 0, 1, 2);
  teleop_layout->addWidget(key_bindings_group);

  teleop_timer_ = new QTimer(this);
  teleop_timer_->setInterval(100);
  connect(teleop_timer_, &QTimer::timeout, this, [this] { update_teleop_motion(); });
  connect(teleop_enable_check_, &QCheckBox::toggled, this, &MainWindow::set_teleop_enabled);
  for (int index = 0; index < TeleopKeyBindings::kStop; ++index) {
    const auto action = static_cast<TeleopKeyBindings::Action>(index);
    connect(teleop_action_buttons_.at(index), &QPushButton::pressed, this,
        [this, action] { set_teleop_button_active(action, true); });
    connect(teleop_action_buttons_.at(index), &QPushButton::released, this,
        [this, action] { set_teleop_button_active(action, false); });
  }
  connect(teleop_action_buttons_.at(TeleopKeyBindings::kStop), &QPushButton::clicked,
      this, &MainWindow::stop_teleop);
  connect(teleop_reset_keys_button_, &QPushButton::clicked, this, [this] {
    key_capture_action_.reset();
    teleop_key_bindings_.reset();
    refresh_key_binding_buttons();
  });
  main_tabs_->addTab(teleop_page_, QString());
  QApplication::instance()->installEventFilter(this);

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
  connect(main_tabs_, &QTabWidget::currentChanged, this, [this](int) { update_controls(); });

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
  stop_teleop();
  stop_local_mapping(true);
  shutdown_rviz();
  bridge_.stop();
}

void MainWindow::changeEvent(QEvent *event) {
  if (event->type() == QEvent::ActivationChange && !isActiveWindow()) {
    set_teleop_enabled(false);
  }
  QMainWindow::changeEvent(event);
}

void MainWindow::closeEvent(QCloseEvent *event) {
  stop_teleop();
  stop_local_mapping(true);
  shutdown_rviz();
  bridge_.stop();
  QMainWindow::closeEvent(event);
}

void MainWindow::focusOutEvent(QFocusEvent *event) {
  set_teleop_enabled(false);
  QMainWindow::focusOutEvent(event);
}

void MainWindow::hideEvent(QHideEvent *event) {
  set_teleop_enabled(false);
  QMainWindow::hideEvent(event);
}

bool MainWindow::eventFilter(QObject *watched, QEvent *event) {
  Q_UNUSED(watched)
  if (!isActiveWindow() || (event->type() != QEvent::KeyPress && event->type() != QEvent::KeyRelease)) {
    return QMainWindow::eventFilter(watched, event);
  }
  auto *key_event = static_cast<QKeyEvent *>(event);
  if (key_capture_action_) {
    if (event->type() == QEvent::KeyPress) {
      keyPressEvent(key_event);
    } else {
      keyReleaseEvent(key_event);
    }
    return true;
  }
  auto *focus = QApplication::focusWidget();
  if (!teleop_enabled_ || qobject_cast<QLineEdit *>(focus) != nullptr ||
      qobject_cast<QAbstractSpinBox *>(focus) != nullptr || qobject_cast<QPlainTextEdit *>(focus) != nullptr ||
      !teleop_key_bindings_.action_for_key(key_event->key())) {
    return QMainWindow::eventFilter(watched, event);
  }
  if (event->type() == QEvent::KeyPress) {
    keyPressEvent(key_event);
  } else {
    keyReleaseEvent(key_event);
  }
  return true;
}

void MainWindow::keyPressEvent(QKeyEvent *event) {
  if (event->isAutoRepeat()) {
    event->accept();
    return;
  }
  if (key_capture_action_) {
    if (event->key() == Qt::Key_Escape) {
      key_capture_action_.reset();
      refresh_key_binding_buttons();
      event->accept();
      return;
    }
    QString error;
    if (!teleop_key_bindings_.set_custom_key(*key_capture_action_, event->key(), &error)) {
      teleop_status_label_->setText(error == QStringLiteral("That key is already assigned") ?
          ui_text("That key is already assigned") : ui_text("Choose a non-modifier key"));
    }
    key_capture_action_.reset();
    refresh_key_binding_buttons();
    event->accept();
    return;
  }
  auto *focus = QApplication::focusWidget();
  if (!teleop_enabled_ || qobject_cast<QLineEdit *>(focus) != nullptr ||
      qobject_cast<QAbstractSpinBox *>(focus) != nullptr || qobject_cast<QPlainTextEdit *>(focus) != nullptr) {
    QMainWindow::keyPressEvent(event);
    return;
  }
  const auto action = teleop_key_bindings_.action_for_key(event->key());
  if (!action) {
    QMainWindow::keyPressEvent(event);
    return;
  }
  if (*action == TeleopKeyBindings::kStop) {
    stop_teleop();
  } else {
    pressed_teleop_keys_.insert(event->key());
    update_teleop_motion();
  }
  event->accept();
}

void MainWindow::keyReleaseEvent(QKeyEvent *event) {
  if (event->isAutoRepeat() || key_capture_action_) {
    event->accept();
    return;
  }
  auto *focus = QApplication::focusWidget();
  if (!teleop_enabled_ || qobject_cast<QLineEdit *>(focus) != nullptr ||
      qobject_cast<QAbstractSpinBox *>(focus) != nullptr || qobject_cast<QPlainTextEdit *>(focus) != nullptr) {
    QMainWindow::keyReleaseEvent(event);
    return;
  }
  if (!teleop_key_bindings_.action_for_key(event->key())) {
    QMainWindow::keyReleaseEvent(event);
    return;
  }
  pressed_teleop_keys_.remove(event->key());
  update_teleop_motion();
  event->accept();
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
      {"Starting local Cartographer mapping", "正在本机启动 Cartographer 建图"},
      {"Local mapping started", "本机建图已启动"},
      {"Stopping local mapping", "正在停止本机建图"},
      {"Local mapping is already running", "本机建图已在运行"},
      {"Local mapping did not start: %1", "本机建图未能启动：%1"},
      {"Local mapping exited with code %1", "本机建图以退出码 %1 结束"},
      {"Local mapping process crashed", "本机建图进程崩溃"},
      {"Prepare the vehicle base stack manually. Mapping runs locally on this Jazzy operator.", "请先手工准备车端基础栈；建图仅在此 Jazzy 操作端本机运行。"},
      {"Mapping prerequisites", "建图前置条件"},
      {"Mapping controls", "建图控制"},
      {"Teleoperation", "遥控"},
      {"Teleoperation is disabled. Enable it only with clear surroundings and a working physical e-stop.", "遥控已禁用。请仅在周围环境清晰且实体急停可用时启用。"},
      {"Teleoperation enable", "启用遥控"},
      {"Teleoperation is ready. Hold a movement button or mapped key to drive.", "遥控已就绪。按住方向按钮或已映射按键即可行驶。"},
      {"Teleoperation is unavailable: start Base or Mapping, wait for vehicle topics, and stop Navigation.", "遥控不可用：请启动基础栈或建图，等待车辆话题就绪，并停止导航。"},
      {"Keyboard bindings", "键盘映射"},
      {"Restore default keys", "恢复默认按键"},
      {"Press a key…", "请按一个按键…"},
      {"Forward", "前进"},
      {"Reverse", "后退"},
      {"Left", "左转"},
      {"Right", "右转"},
      {"%1 (hold)", "%1（按住）"},
      {"Choose a non-modifier key", "请选择非修饰键"},
      {"That key is already assigned", "该按键已被分配"},
      {"Map name", "地图名称"},
      {"Linear speed (m/s)", "线速度（米/秒）"},
      {"Turn speed (rad/s)", "转向速度（弧度/秒）"},
      {"Forward (hold)", "前进（按住）"},
      {"Reverse (hold)", "后退（按住）"},
      {"Left (hold)", "左转（按住）"},
      {"Right (hold)", "右转（按住）"},
      {"Stop", "停止"},
      {"Teleoperation stopped", "遥控已停止"},
      {"Save Map", "保存地图"},
      {"Stop Mapping", "停止建图"},
      {"Stop navigation before starting mapping", "请先停止导航，再启动建图"},
      {"Vehicle base topics are not ready; start the vehicle base stack manually", "车辆基础话题未就绪；请手工启动车端基础栈"},
      {"Mapping stopped", "建图已停止"},
      {"Save map before stopping?", "停止前保存地图？"},
      {"This mapping result has not been saved. Save it before stopping?", "当前建图结果尚未保存。是否先保存再停止？"},
      {"A running mapping session and an active /map topic are required", "需要正在运行的建图任务以及活跃的 /map 话题"},
      {"Map name must contain only letters, numbers, underscores, or hyphens", "地图名称只能包含字母、数字、下划线或短横线"},
      {"Navigation map directory is not writable", "导航地图目录不可写"},
      {"Replace existing map?", "覆盖已有地图？"},
      {"A map with this name already exists. Replace it?", "已存在同名地图，是否覆盖？"},
      {"Map save failed", "地图保存失败"},
      {"Map save did not start: %1", "地图保存未能启动：%1"},
      {"Map saved: %1", "地图已保存：%1"},
      {"Saving map: %1", "正在保存地图：%1"},
      {"Map save is already in progress", "地图正在保存中"},
      {"Status: %1", "状态：%1"},
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
  main_tabs_->setTabText(main_tabs_->indexOf(mapping_page_), ui_text("Mapping"));
  main_tabs_->setTabText(main_tabs_->indexOf(teleop_page_), ui_text("Teleoperation"));
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
  stop_all_button_->setText(ui_text("Stop All"));
  set_initial_pose_button_->setText(ui_text("Set Initial Pose"));
  send_goal_button_->setText(ui_text("Send Goal"));
  cancel_goal_button_->setText(ui_text("Cancel Goal"));
  start_patrol_button_->setText(ui_text("Start Patrol"));
  cancel_patrol_button_->setText(ui_text("Cancel Patrol"));
  mapping_hint_->setText(ui_text(
      "Prepare the vehicle base stack manually. Mapping runs locally on this Jazzy operator."));
  if (auto *group = mapping_page_->findChild<QGroupBox *>("mappingHealthGroup")) {
    group->setTitle(ui_text("Mapping prerequisites"));
  }
  if (auto *group = mapping_page_->findChild<QGroupBox *>("mappingControlsGroup")) {
    group->setTitle(ui_text("Mapping controls"));
  }
  mapping_name_label_->setText(ui_text("Map name"));
  mapping_start_button_->setText(ui_text("Start Mapping"));
  mapping_save_button_->setText(ui_text("Save Map"));
  mapping_stop_button_->setText(ui_text("Stop Mapping"));
  teleop_hint_->setText(ui_text(
      "Teleoperation is disabled. Enable it only with clear surroundings and a working physical e-stop."));
  if (auto *group = teleop_page_->findChild<QGroupBox *>("teleopEnableGroup")) {
    group->setTitle(ui_text("Teleoperation"));
  }
  if (auto *group = teleop_page_->findChild<QGroupBox *>("teleopControlsGroup")) {
    group->setTitle(ui_text("Teleoperation"));
  }
  if (auto *group = teleop_page_->findChild<QGroupBox *>("teleopKeyBindingsGroup")) {
    group->setTitle(ui_text("Keyboard bindings"));
  }
  teleop_enable_check_->setText(ui_text("Teleoperation enable"));
  teleop_linear_label_->setText(ui_text("Linear speed (m/s)"));
  teleop_angular_label_->setText(ui_text("Turn speed (rad/s)"));
  for (int index = 0; index < TeleopKeyBindings::kActionCount; ++index) {
    const auto action = static_cast<TeleopKeyBindings::Action>(index);
    teleop_action_buttons_.at(index)->setText(
        action == TeleopKeyBindings::kStop ? ui_text("Stop") :
        ui_text("%1 (hold)").arg(teleop_action_text(action)));
  }
  teleop_reset_keys_button_->setText(ui_text("Restore default keys"));
  refresh_key_binding_buttons();
  if (component_display_.find("mapping") == component_display_.end()) {
    mapping_state_label_->setText(ui_text("Status: %1").arg(state_text(console_state_.mapping_state)));
  }

  safety_hint_->setText(ui_text(
      "Start Base first, then choose one high-level mode. Mapping and navigation are mutually exclusive."));
  update_mode_summary();

  for (const auto &entry : component_labels_) {
    update_component_label(entry.first);
  }
  for (const auto &entry : health_labels_) {
    update_health_label(entry.first);
  }
  for (const auto &entry : mapping_health_labels_) {
    update_health_label(entry.first);
  }
  update_patrol_progress_label();
  update_controls();
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
  const auto activity = health_activity_.find(topic);
  const QString text = activity == health_activity_.end() ? ui_text("Unknown") :
      (activity->second ? ui_text("Active") : ui_text("Inactive"));
  const QString style = activity != health_activity_.end() && activity->second ?
      "color: #197a4a; font-weight: 600;" : "color: #8a4b08; font-weight: 600;";
  for (const auto *labels : {&health_labels_, &mapping_health_labels_}) {
    const auto label = labels->find(topic);
    if (label != labels->end()) {
      label->second->setText(text);
      label->second->setStyleSheet(style);
    }
  }
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
  start_navigation_button_->setEnabled(
      console_state_.can_start_navigation() && map_input_->count() > 0 && !console_state_.is_mapping());
  mapping_start_button_->setEnabled(console_state_.can_start_mapping() && !navigation_ready);
  const bool mapping_running = console_state_.is_mapping();
  const bool map_available = health_activity_.find("/map") != health_activity_.end() && health_activity_.at("/map");
  mapping_save_button_->setEnabled(mapping_running && map_available &&
      (map_save_process_ == nullptr || map_save_process_->state() == QProcess::NotRunning));
  mapping_stop_button_->setEnabled(mapping_process_ != nullptr &&
      mapping_process_->state() != QProcess::NotRunning);
  const bool teleop_available = teleop_prerequisites_met();
  if (!teleop_available && teleop_enabled_) {
    set_teleop_enabled(false);
  }
  const bool teleop_ready = teleop_available && teleop_enabled_;
  teleop_enable_check_->setEnabled(teleop_available);
  for (auto *button : teleop_action_buttons_) {
    button->setEnabled(teleop_ready);
  }
  teleop_linear_input_->setEnabled(teleop_ready);
  teleop_angular_input_->setEnabled(teleop_ready);
  for (auto *button : key_capture_buttons_) {
    button->setEnabled(!teleop_enabled_);
  }
  teleop_reset_keys_button_->setEnabled(!teleop_enabled_);
  teleop_status_label_->setText(teleop_ready ?
      ui_text("Teleoperation is ready. Hold a movement button or mapped key to drive.") :
      ui_text("Teleoperation is unavailable: start Base or Mapping, wait for vehicle topics, and stop Navigation."));
  set_initial_pose_button_->setEnabled(navigation_ready);
  send_goal_button_->setEnabled(navigation_ready);
  start_patrol_button_->setEnabled(navigation_ready && route_input_->count() > 0);
}

void MainWindow::refresh_assets(const QString &preferred_map) {
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

  const QString requested_map = preferred_map.isEmpty() ? current_map : preferred_map;
  const int map_index = map_input_->findText(requested_map);
  map_input_->setCurrentIndex(map_index >= 0 ? map_index : 0);
  const int route_index = route_input_->findText(current_route);
  route_input_->setCurrentIndex(route_index >= 0 ? route_index : 0);
  update_controls();
}

void MainWindow::append_mapping_log(const QString &message) {
  mapping_log_->appendPlainText(QString("[%1] %2").arg(
      QDateTime::currentDateTime().toString("HH:mm:ss"), message));
}

QString MainWindow::mapping_output_base() const {
  return QString::fromStdString(ament_index_cpp::get_package_share_directory("fishbot_navigation2")) +
      "/maps/" + mapping_name_input_->text().trimmed();
}

void MainWindow::start_local_mapping() {
  if (console_state_.can_navigate()) {
    append_mapping_log(ui_text("Stop navigation before starting mapping"));
    return;
  }
  for (const QString &topic : {QString("/scan"), QString("/odom"), QString("/imu"), QString("/tf")}) {
    if (health_activity_.find(topic) == health_activity_.end() || !health_activity_.at(topic)) {
      append_mapping_log(ui_text("Vehicle base topics are not ready; start the vehicle base stack manually"));
      return;
    }
  }
  if (mapping_process_ == nullptr) {
    mapping_process_ = new QProcess(this);
    mapping_process_->setProcessChannelMode(QProcess::MergedChannels);
    connect(mapping_process_, &QProcess::started, this, [this] {
      update_component("mapping", ConsoleState::kRunning,
          static_cast<int>(mapping_process_->processId()), ui_text("Local mapping started"));
      append_mapping_log(ui_text("Local mapping started"));
    });
    connect(mapping_process_, &QProcess::readyReadStandardOutput, this, [this] {
      const QString output = QString::fromLocal8Bit(mapping_process_->readAllStandardOutput()).trimmed();
      if (!output.isEmpty()) {
        append_mapping_log(output);
      }
    });
    connect(mapping_process_, &QProcess::errorOccurred, this,
        [this](QProcess::ProcessError error) {
          if (error == QProcess::FailedToStart) {
            update_component("mapping", ConsoleState::kError, 0,
                ui_text("Local mapping did not start: %1").arg(mapping_process_->errorString()));
            append_mapping_log(ui_text("Local mapping did not start: %1").arg(mapping_process_->errorString()));
          }
        });
    connect(mapping_process_, qOverload<int, QProcess::ExitStatus>(&QProcess::finished), this,
        [this](int exit_code, QProcess::ExitStatus exit_status) {
          stop_teleop();
          const bool requested_stop = mapping_stop_requested_;
          mapping_stop_requested_ = false;
          if (requested_stop) {
            update_component("mapping", ConsoleState::kStopped, 0, ui_text("Stopped"));
            append_mapping_log(ui_text("Mapping stopped"));
            return;
          }
          const QString detail = exit_status == QProcess::NormalExit
              ? ui_text("Local mapping exited with code %1").arg(exit_code)
              : ui_text("Local mapping process crashed");
          update_component("mapping", ConsoleState::kError, 0, detail);
          append_mapping_log(detail);
        });
  }

  if (mapping_process_->state() != QProcess::NotRunning) {
    append_log(ui_text("Local mapping is already running"));
    return;
  }

  mapping_stop_requested_ = false;
  mapping_has_unsaved_changes_ = true;
  update_component("mapping", ConsoleState::kStarting, 0, ui_text("Starting local Cartographer mapping"));
  append_mapping_log(ui_text("Starting local Cartographer mapping"));
  mapping_process_->start("ros2", {"launch", "fishbot_cartographer", "cartographer.launch.py",
      "use_sim_time:=false"});
}

void MainWindow::request_stop_local_mapping() {
  if (mapping_process_ == nullptr || mapping_process_->state() == QProcess::NotRunning) {
    return;
  }
  if (!mapping_has_unsaved_changes_) {
    stop_local_mapping(false);
    return;
  }
  const auto response = QMessageBox::question(this, ui_text("Save map before stopping?"),
      ui_text("This mapping result has not been saved. Save it before stopping?"),
      QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel, QMessageBox::Save);
  if (response == QMessageBox::Save) {
    stop_after_map_save_ = true;
    save_current_map();
  } else if (response == QMessageBox::Discard) {
    stop_local_mapping(true);
  }
}

void MainWindow::stop_local_mapping(bool discard_unsaved) {
  stop_teleop();
  if (mapping_process_ == nullptr || mapping_process_->state() == QProcess::NotRunning) {
    return;
  }
  if (!discard_unsaved && mapping_has_unsaved_changes_) {
    return;
  }
  mapping_stop_requested_ = true;
  update_component("mapping", ConsoleState::kStopping,
      static_cast<int>(mapping_process_->processId()), ui_text("Stopping local mapping"));
  mapping_process_->terminate();
  QTimer::singleShot(5000, mapping_process_, [this] {
    if (mapping_process_ != nullptr && mapping_process_->state() != QProcess::NotRunning) {
      mapping_process_->kill();
    }
  });
}

bool MainWindow::vehicle_topics_ready() const {
  for (const QString &topic : {QString("/scan"), QString("/odom"), QString("/imu"), QString("/tf")}) {
    const auto activity = health_activity_.find(topic);
    if (activity == health_activity_.end() || !activity->second) {
      return false;
    }
  }
  return true;
}

bool MainWindow::teleop_prerequisites_met() const {
  const auto bringup = component_display_.find("bringup");
  const bool base_running = bringup != component_display_.end() &&
      bringup->second.state == ConsoleState::kRunning;
  return !console_state_.can_navigate() && (base_running || console_state_.is_mapping()) &&
      vehicle_topics_ready();
}

void MainWindow::set_teleop_enabled(bool enabled) {
  if (enabled && !teleop_prerequisites_met()) {
    enabled = false;
  }
  if (teleop_enable_check_->isChecked() != enabled) {
    const QSignalBlocker blocker(teleop_enable_check_);
    teleop_enable_check_->setChecked(enabled);
  }
  teleop_enabled_ = enabled;
  if (!teleop_enabled_) {
    stop_teleop();
  }
  update_controls();
}

void MainWindow::set_teleop_button_active(TeleopKeyBindings::Action action, bool active) {
  if (!teleop_enabled_) {
    return;
  }
  pressed_teleop_buttons_.at(action) = active;
  update_teleop_motion();
}

void MainWindow::update_teleop_motion() {
  if (!teleop_enabled_ || !teleop_prerequisites_met()) {
    set_teleop_enabled(false);
    return;
  }
  const auto pressed = [this](TeleopKeyBindings::Action action) {
    return pressed_teleop_buttons_.at(action) || teleop_key_bindings_.is_pressed(action, pressed_teleop_keys_);
  };
  const int linear = (pressed(TeleopKeyBindings::kForward) ? 1 : 0) -
      (pressed(TeleopKeyBindings::kReverse) ? 1 : 0);
  const int angular = (pressed(TeleopKeyBindings::kLeft) ? 1 : 0) -
      (pressed(TeleopKeyBindings::kRight) ? 1 : 0);
  teleop_linear_x_ = linear * teleop_linear_input_->value();
  teleop_angular_z_ = angular * teleop_angular_input_->value();
  if (linear == 0 && angular == 0) {
    teleop_timer_->stop();
    bridge_.stop_teleop();
    return;
  }
  bridge_.publish_teleop_velocity(teleop_linear_x_, teleop_angular_z_);
  teleop_timer_->start();
}

void MainWindow::stop_teleop() {
  if (teleop_timer_ != nullptr) {
    teleop_timer_->stop();
  }
  pressed_teleop_keys_.clear();
  pressed_teleop_buttons_.fill(false);
  teleop_linear_x_ = 0.0;
  teleop_angular_z_ = 0.0;
  bridge_.stop_teleop();
}

QString MainWindow::teleop_action_text(TeleopKeyBindings::Action action) const {
  switch (action) {
    case TeleopKeyBindings::kForward:
      return ui_text("Forward");
    case TeleopKeyBindings::kReverse:
      return ui_text("Reverse");
    case TeleopKeyBindings::kLeft:
      return ui_text("Left");
    case TeleopKeyBindings::kRight:
      return ui_text("Right");
    case TeleopKeyBindings::kStop:
      return ui_text("Stop");
    case TeleopKeyBindings::kActionCount:
      return {};
  }
  return {};
}

void MainWindow::refresh_key_binding_buttons() {
  for (int index = 0; index < TeleopKeyBindings::kActionCount; ++index) {
    const auto action = static_cast<TeleopKeyBindings::Action>(index);
    auto *button = key_capture_buttons_.at(index);
    if (key_capture_action_ && *key_capture_action_ == action) {
      button->setText(ui_text("Press a key…"));
      continue;
    }
    QStringList names;
    for (const int key : teleop_key_bindings_.keys_for(action)) {
      names.append(QKeySequence(key).toString(QKeySequence::NativeText));
    }
    button->setText(QString("%1: %2").arg(teleop_action_text(action), names.join(" / ")));
  }
}

void MainWindow::begin_key_capture(TeleopKeyBindings::Action action) {
  set_teleop_enabled(false);
  key_capture_action_ = action;
  refresh_key_binding_buttons();
  key_capture_buttons_.at(action)->setFocus(Qt::OtherFocusReason);
}

void MainWindow::save_current_map() {
  if (mapping_process_ == nullptr || mapping_process_->state() != QProcess::Running ||
      health_activity_.find("/map") == health_activity_.end() || !health_activity_.at("/map")) {
    append_mapping_log(ui_text("A running mapping session and an active /map topic are required"));
    stop_after_map_save_ = false;
    return;
  }
  const QString map_name = mapping_name_input_->text().trimmed();
  static const QRegularExpression valid_name("^[A-Za-z0-9_-]+$");
  if (!valid_name.match(map_name).hasMatch()) {
    append_mapping_log(ui_text("Map name must contain only letters, numbers, underscores, or hyphens"));
    stop_after_map_save_ = false;
    return;
  }
  const QFileInfo output_directory(QFileInfo(mapping_output_base()).dir().absolutePath());
  if (!output_directory.isDir() || !output_directory.isWritable()) {
    append_mapping_log(ui_text("Navigation map directory is not writable"));
    stop_after_map_save_ = false;
    return;
  }
  const QString output_base = mapping_output_base();
  if (QFileInfo::exists(output_base + ".yaml") || QFileInfo::exists(output_base + ".pgm")) {
    if (QMessageBox::question(this, ui_text("Replace existing map?"),
        ui_text("A map with this name already exists. Replace it?"),
        QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel) != QMessageBox::Yes) {
      stop_after_map_save_ = false;
      return;
    }
  }
  if (map_save_process_ == nullptr) {
    map_save_process_ = new QProcess(this);
    map_save_process_->setProcessChannelMode(QProcess::MergedChannels);
    connect(map_save_process_, &QProcess::readyReadStandardOutput, this, [this] {
      const QString output = QString::fromLocal8Bit(map_save_process_->readAllStandardOutput()).trimmed();
      if (!output.isEmpty()) {
        append_mapping_log(output);
      }
    });
    connect(map_save_process_, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
      if (error == QProcess::FailedToStart) {
        append_mapping_log(ui_text("Map save did not start: %1").arg(map_save_process_->errorString()));
        stop_after_map_save_ = false;
        update_controls();
      }
    });
    connect(map_save_process_, qOverload<int, QProcess::ExitStatus>(&QProcess::finished), this,
        [this](int exit_code, QProcess::ExitStatus exit_status) {
          const QString output_base = map_save_process_->property("output_base").toString();
          const QString map_file = map_save_process_->property("map_file").toString();
          const bool saved = exit_status == QProcess::NormalExit && exit_code == 0 &&
              QFileInfo::exists(output_base + ".yaml") && QFileInfo::exists(output_base + ".pgm");
          if (!saved) {
            append_mapping_log(ui_text("Map save failed"));
            stop_after_map_save_ = false;
            update_controls();
            return;
          }
          mapping_has_unsaved_changes_ = false;
          append_mapping_log(ui_text("Map saved: %1").arg(map_file));
          refresh_assets(map_file);
          update_controls();
          if (stop_after_map_save_) {
            stop_after_map_save_ = false;
            stop_local_mapping(false);
          }
        });
  }
  if (map_save_process_->state() != QProcess::NotRunning) {
    append_mapping_log(ui_text("Map save is already in progress"));
    return;
  }
  append_mapping_log(ui_text("Saving map: %1").arg(map_name));
  map_save_process_->setProperty("output_base", output_base);
  map_save_process_->setProperty("map_file", map_name + ".yaml");
  map_save_process_->start("ros2", {"run", "nav2_map_server", "map_saver_cli", "-f", output_base});
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
    mapping_state_label_->setText(ui_text("%1: %2").arg(state_text(state), detail));
  }
  update_mode_summary();
  update_controls();
  append_log(ui_text("%1: %2").arg(component, detail));
}

void MainWindow::update_health(const QString &topic, bool active) {
  health_activity_[topic] = active;
  update_health_label(topic);
  update_controls();
}

}  // namespace car_operator_console
