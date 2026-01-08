#ifndef ROBOT_ARM_GUI_RVIZ_PANEL_HPP
#define ROBOT_ARM_GUI_RVIZ_PANEL_HPP

#include <QWidget>
#include <QVBoxLayout>
#include <memory>

#include <rclcpp/rclcpp.hpp>
#include <rviz_common/render_panel.hpp>
#include <rviz_common/visualization_manager.hpp>
#include <rviz_common/display.hpp>
#include <rviz_common/tool_manager.hpp>
#include <rviz_common/view_manager.hpp>
#include <rviz_common/ros_integration/ros_node_abstraction_iface.hpp>

class RvizPanel : public QWidget
{
    Q_OBJECT

public:
    explicit RvizPanel(rclcpp::Node::SharedPtr node, QWidget* parent = nullptr);
    ~RvizPanel() override;

    // 初始化（必须在显示后调用）
    void initialize();
    
    // 添加显示类型
    void addRobotModel(const std::string& robot_description_topic = "/robot_description");
    void addGrid();
    void addTF();
    void addAxes();

    // 获取 VisualizationManager（用于高级操作）
    rviz_common::VisualizationManager* getManager() { return manager_; }

private:
    void setupRviz();

    rclcpp::Node::SharedPtr node_;
    
    // RViz 核心组件
    rviz_common::RenderPanel* render_panel_;
    rviz_common::VisualizationManager* manager_;
    
    QVBoxLayout* layout_;
    bool initialized_;
};

#endif // ROBOT_ARM_GUI_RVIZ_PANEL_HPP
