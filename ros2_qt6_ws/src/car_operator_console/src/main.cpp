#include <QApplication>
#include <QCommandLineOption>
#include <QCommandLineParser>

#include <rclcpp/rclcpp.hpp>

#include "car_operator_console/main_window.hpp"

int main(int argc, char *argv[]) {
  rclcpp::init(argc, argv);
  QApplication application(argc, argv);
  QCoreApplication::setOrganizationName("ROS 2 Car");
  QCoreApplication::setOrganizationDomain("ros2-car.local");
  QCoreApplication::setApplicationName("ROS 2 Car Console");
  QCommandLineParser parser;
  parser.setApplicationDescription("Qt 5 console with embedded RViz for the ROS 2 car");
  parser.addHelpOption();
  QCommandLineOption mode_option(
      {"m", "mode"}, "Console placement: vehicle or operator.", "mode", "operator");
  parser.addOption(mode_option);
  parser.process(application);

  int result = 0;
  {
    car_operator_console::MainWindow window(parser.value(mode_option));
    window.show();
    result = application.exec();
  }
  rclcpp::shutdown();
  return result;
}
