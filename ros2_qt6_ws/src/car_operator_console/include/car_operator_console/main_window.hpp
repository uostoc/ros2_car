#pragma once

#include <map>
#include <memory>
#include <vector>

#include <QMainWindow>

#include "car_operator_console/console_state.hpp"
#include "car_operator_console/language_manager.hpp"
#include "car_operator_console/ros_bridge.hpp"

class QAction;
class QActionGroup;
class QComboBox;
class QCloseEvent;
class QDoubleSpinBox;
class QGroupBox;
class QLabel;
class QLineEdit;
class QMenu;
class QPlainTextEdit;
class QPushButton;
class QProcess;
class QTabWidget;
class QTimer;
class QVBoxLayout;
class QWidget;

namespace rviz_common {
class VisualizationFrame;
namespace ros_integration {
class RosNodeAbstraction;
}
}  // namespace rviz_common

namespace car_operator_console {

class MainWindow final : public QMainWindow {
  Q_OBJECT

 public:
  explicit MainWindow(
      const QString &mode, QWidget *parent = nullptr, const QString &translations_directory = {});
  ~MainWindow() override;

 protected:
  void closeEvent(QCloseEvent *event) override;

 private:
  struct ComponentDisplay {
    int state{};
    int pid{};
    QString detail;
  };

  void append_log(const QString &message);
  QString ui_text(const char *source) const;
  void retranslate_ui();
  void update_component_label(const QString &component);
  void update_health_label(const QString &topic);
  void update_patrol_progress_label();
  void update_mode_summary();
  void update_controls();
  void refresh_assets(const QString &preferred_map = {});
  void append_mapping_log(const QString &message);
  void start_local_mapping();
  void request_stop_local_mapping();
  void stop_local_mapping(bool discard_unsaved);
  void save_current_map();
  QString mapping_output_base() const;
  void start_teleop(double linear_x, double angular_z);
  void stop_teleop();
  void initialize_rviz();
  void shutdown_rviz();
  bool confirm_mode_switch(const QString &target_mode);
  void update_component(const QString &component, int state, int pid, const QString &detail);
  void update_health(const QString &topic, bool active);
  QString state_text(int state) const;

  RosBridge bridge_;
  LanguageManager language_manager_;
  ConsoleState console_state_;
  QString mode_;
  std::map<QString, QLabel *> component_labels_;
  std::map<QString, QLabel *> health_labels_;
  std::map<QString, QLabel *> mapping_health_labels_;
  std::map<QString, ComponentDisplay> component_display_;
  std::map<QString, bool> health_activity_;
  QGroupBox *health_group_{};
  QGroupBox *stack_group_{};
  QGroupBox *navigation_group_{};
  QGroupBox *patrol_group_{};
  QLabel *mode_summary_{};
  QLabel *safety_hint_{};
  QLabel *map_label_{};
  QLabel *x_label_{};
  QLabel *y_label_{};
  QLabel *yaw_label_{};
  QLabel *route_label_{};
  QDoubleSpinBox *x_input_{};
  QDoubleSpinBox *y_input_{};
  QDoubleSpinBox *yaw_input_{};
  QComboBox *map_input_{};
  QComboBox *route_input_{};
  QPushButton *start_base_button_{};
  QPushButton *start_navigation_button_{};
  QPushButton *stop_all_button_{};
  QPushButton *set_initial_pose_button_{};
  QPushButton *send_goal_button_{};
  QPushButton *cancel_goal_button_{};
  QPushButton *start_patrol_button_{};
  QPushButton *cancel_patrol_button_{};
  QPushButton *refresh_assets_button_{};
  QLabel *patrol_progress_{};
  QPlainTextEdit *log_{};
  QProcess *mapping_process_{};
  QProcess *map_save_process_{};
  QTabWidget *main_tabs_{};
  QWidget *mapping_page_{};
  QLabel *mapping_hint_{};
  QLabel *mapping_state_label_{};
  QLabel *mapping_name_label_{};
  QLabel *teleop_linear_label_{};
  QLabel *teleop_angular_label_{};
  QLineEdit *mapping_name_input_{};
  QDoubleSpinBox *teleop_linear_input_{};
  QDoubleSpinBox *teleop_angular_input_{};
  QPushButton *mapping_start_button_{};
  QPushButton *mapping_save_button_{};
  QPushButton *mapping_stop_button_{};
  std::vector<QPushButton *> teleop_buttons_;
  QPlainTextEdit *mapping_log_{};
  QTimer *teleop_timer_{};
  QWidget *rviz_page_{};
  QVBoxLayout *rviz_layout_{};
  QLabel *rviz_placeholder_{};
  rviz_common::VisualizationFrame *rviz_frame_{};
  std::shared_ptr<rviz_common::ros_integration::RosNodeAbstraction> rviz_node_;
  QMenu *language_menu_{};
  QActionGroup *language_action_group_{};
  QAction *english_action_{};
  QAction *simplified_chinese_action_{};
  bool has_patrol_progress_{false};
  unsigned int patrol_current_{};
  unsigned int patrol_total_{};
  QString patrol_point_;
  bool rviz_initialization_attempted_{false};
  bool mapping_stop_requested_{false};
  bool mapping_has_unsaved_changes_{false};
  bool stop_after_map_save_{false};
  double teleop_linear_x_{0.0};
  double teleop_angular_z_{0.0};
};

}  // namespace car_operator_console
