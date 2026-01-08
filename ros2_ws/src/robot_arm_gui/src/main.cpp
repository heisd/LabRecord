#include <QApplication>
#include <QTimer>
#include <thread>
#include <memory>

#include <rclcpp/rclcpp.hpp>
#include "robot_arm_gui/main_window.hpp"

int main(int argc, char** argv)
{
    // 初始化 ROS2
    rclcpp::init(argc, argv);
    
    // 创建节点
    auto node = std::make_shared<rclcpp::Node>("robot_arm_gui");
    
    // 初始化 Qt（必须在 ROS2 之后）
    QApplication app(argc, argv);
    app.setApplicationName("Robot Arm GUI");
    
    // 创建主窗口
    MainWindow window(node);
    window.setWindowTitle("机器臂控制 - RViz2 嵌入版");
    window.resize(1200, 800);
    window.show();
    
    // 延迟初始化 RViz（需要窗口先显示）
    QTimer::singleShot(100, &window, &MainWindow::initializeRviz);
    
    // ROS2 spin 在单独线程
    std::thread ros_thread([&node]() {
        rclcpp::spin(node);
    });
    
    // 运行 Qt 事件循环
    int result = app.exec();
    
    // 清理
    rclcpp::shutdown();
    if (ros_thread.joinable()) {
        ros_thread.join();
    }
    
    return result;
}
