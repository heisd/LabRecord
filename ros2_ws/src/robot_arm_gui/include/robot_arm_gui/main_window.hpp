#ifndef ROBOT_ARM_GUI_MAIN_WINDOW_HPP
#define ROBOT_ARM_GUI_MAIN_WINDOW_HPP

#include <QMainWindow>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <memory>

#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/joint_state.hpp>

// 前向声明
class RvizPanel;
class JointControlPanel;

namespace rviz_common {
class RenderPanel;
class VisualizationManager;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(rclcpp::Node::SharedPtr node, QWidget* parent = nullptr);
    ~MainWindow() override;

    // 初始化 RViz（必须在 Qt 事件循环启动后调用）
    void initializeRviz();

public slots:
    // 关节角度变化时调用
    void onJointAnglesChanged(const std::vector<double>& angles_deg);
    
    // 归零
    void onGoHome();
    
    // 准备位
    void onGoReady();

private:
    void setupUi();
    void publishJointStates();

    // ROS2 节点
    rclcpp::Node::SharedPtr node_;
    rclcpp::Publisher<sensor_msgs::msg::JointState>::SharedPtr joint_pub_;
    rclcpp::TimerBase::SharedPtr timer_;

    // UI 组件
    RvizPanel* rviz_panel_;
    JointControlPanel* joint_panel_;
    
    // 当前关节角度（弧度）
    std::vector<double> current_joint_angles_;
    std::vector<std::string> joint_names_;
};

#endif // ROBOT_ARM_GUI_MAIN_WINDOW_HPP
