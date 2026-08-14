#pragma once

#include <map>

#include <QMainWindow>

#include "car_operator_console/console_state.hpp"
#include "car_operator_console/ros_bridge.hpp"

class QComboBox;
class QCloseEvent;
class QDoubleSpinBox;
class QLabel;
class QPlainTextEdit;

namespace car_operator_console {

class MainWindow final : public QMainWindow {
  Q_OBJECT

 public:
  explicit MainWindow(const QString &mode, QWidget *parent = nullptr);
  ~MainWindow() override;

 protected:
  void closeEvent(QCloseEvent *event) override;

 private:
  void append_log(const QString &message);
  void update_component(const QString &component, int state, int pid, const QString &detail);
  void update_health(const QString &topic, bool active);
  static QString state_text(int state);

  RosBridge bridge_;
  ConsoleState console_state_;
  std::map<QString, QLabel *> component_labels_;
  std::map<QString, QLabel *> health_labels_;
  QDoubleSpinBox *x_input_{};
  QDoubleSpinBox *y_input_{};
  QDoubleSpinBox *yaw_input_{};
  QComboBox *route_input_{};
  QLabel *patrol_progress_{};
  QPlainTextEdit *log_{};
};

}  // namespace car_operator_console
