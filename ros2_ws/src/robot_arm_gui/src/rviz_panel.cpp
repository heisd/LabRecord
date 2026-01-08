#include "robot_arm_gui/rviz_panel.hpp"

#include <rviz_common/display_group.hpp>
#include <rviz_common/display_context.hpp>
#include <rviz_common/window_manager_interface.hpp>
#include <rviz_common/ros_integration/ros_node_abstraction.hpp>
#include <rviz_rendering/render_window.hpp>

#include <QVBoxLayout>
#include <QDebug>
#include <QApplication>
#include <cstdio>

// 简单的窗口管理器实现
class DummyWindowManager : public rviz_common::WindowManagerInterface
{
public:
    QWidget* getParentWindow() override { return nullptr; }
    
    rviz_common::PanelDockWidget* addPane(
        const QString& /*name*/,
        QWidget* /*pane*/,
        Qt::DockWidgetArea /*area*/ = Qt::LeftDockWidgetArea,
        bool /*floating*/ = false) override
    {
        return nullptr;
    }
    
    void setStatus(const QString& /*message*/) override {}
};

// ROS 节点抽象层包装器
class RosNodeAbstractionWrapper : public rviz_common::ros_integration::RosNodeAbstractionIface
{
public:
    explicit RosNodeAbstractionWrapper(rclcpp::Node::SharedPtr node)
        : node_(node)
    {}

    std::string get_node_name() const override {
        return node_->get_name();
    }

    // get_namespace 不是基类的虚函数，所以不能用 override
    std::string get_namespace() const {
        return node_->get_namespace();
    }

    rclcpp::Node::SharedPtr get_raw_node() override {
        return node_;
    }

    // 实现缺失的纯虚函数 get_topic_names_and_types
    std::map<std::string, std::vector<std::string>> get_topic_names_and_types() const override {
        return node_->get_topic_names_and_types();
    }

private:
    rclcpp::Node::SharedPtr node_;
};

RvizPanel::RvizPanel(rclcpp::Node::SharedPtr node, QWidget* parent)
    : QWidget(parent)
    , node_(node)
    , render_panel_(nullptr)
    , manager_(nullptr)
    , layout_(nullptr)
    , initialized_(false)
{
    layout_ = new QVBoxLayout(this);
    layout_->setContentsMargins(0, 0, 0, 0);

    // 注意：不在构造函数中创建 RenderPanel
    // 延迟到 initialize() 中创建，确保 OpenGL 上下文已就绪
    qDebug() << "RvizPanel constructor completed";
}

RvizPanel::~RvizPanel()
{
    if (manager_) {
        manager_->stopUpdate();
        delete manager_;
        manager_ = nullptr;
    }
}

void RvizPanel::initialize()
{
    if (initialized_) return;
    
    try {
        setupRviz();
        initialized_ = true;
        qDebug() << "RViz panel initialized successfully";
    } catch (const std::exception& e) {
        qWarning() << "Failed to initialize RViz:" << e.what();
    }
}

void RvizPanel::setupRviz()
{
    qDebug() << "[RViz] Step 1: Creating RenderPanel...";
    fflush(stdout);

    // 在这里创建 RenderPanel（窗口已显示，OpenGL 上下文应该就绪）
    render_panel_ = new rviz_common::RenderPanel(this);
    layout_->addWidget(render_panel_);

    qDebug() << "[RViz] Step 1b: Showing RenderPanel...";
    fflush(stdout);

    // 确保 widget 已经被处理
    render_panel_->show();
    QApplication::processEvents();

    qDebug() << "[RViz] Step 2: Creating clock and window manager...";
    fflush(stdout);

    // 创建时钟
    auto clock = node_->get_clock();

    // 创建窗口管理器
    auto window_manager = new DummyWindowManager();

    qDebug() << "[RViz] Step 2b: Creating RosNodeAbstraction (official RViz class)...";
    fflush(stdout);

    // 使用 RViz 官方的 RosNodeAbstraction 类
    auto ros_node_abstraction = std::make_shared<rviz_common::ros_integration::RosNodeAbstraction>("rviz_embedded");

    qDebug() << "[RViz] Step 3: Creating VisualizationManager...";
    qDebug() << "[RViz] render_panel_ = " << (void*)render_panel_;
    qDebug() << "[RViz] ros_node_abstraction = " << (void*)ros_node_abstraction.get();
    qDebug() << "[RViz] window_manager = " << (void*)window_manager;
    qDebug() << "[RViz] clock = " << (void*)clock.get();
    fflush(stdout);

    // 创建 VisualizationManager
    manager_ = new rviz_common::VisualizationManager(
        render_panel_,
        ros_node_abstraction,
        window_manager,
        clock);

    qDebug() << "[RViz] Step 4: VisualizationManager created, now initializing render_panel...";
    fflush(stdout);

    // 用 manager 初始化 render_panel
    render_panel_->initialize(manager_);

    qDebug() << "[RViz] Step 5: Initializing render window...";
    fflush(stdout);

    // 初始化渲染窗口
    auto render_window = render_panel_->getRenderWindow();
    if (render_window) {
        render_window->initialize();
        qDebug() << "[RViz] Render window initialized";
    }

    qDebug() << "[RViz] Step 6: Initializing manager...";
    fflush(stdout);

    // 初始化管理器
    manager_->initialize();

    qDebug() << "[RViz] Step 7: Starting update...";
    fflush(stdout);

    manager_->startUpdate();

    // 设置固定帧
    manager_->setFixedFrame("base_link");

    qDebug() << "[RViz] Setup completed!";
    fflush(stdout);
}

void RvizPanel::addRobotModel(const std::string& robot_description_topic)
{
    qDebug() << "[RobotModel] Starting addRobotModel...";
    fflush(stdout);

    if (!manager_) {
        qWarning() << "Manager not initialized";
        return;
    }

    try {
        qDebug() << "[RobotModel] Creating RobotModel display (disabled first)...";
        fflush(stdout);

        // 先创建禁用的 display，避免在设置属性时崩溃
        auto display = manager_->createDisplay(
            "rviz_default_plugins/RobotModel",
            "RobotModel",
            false);  // 先禁用

        qDebug() << "[RobotModel] Display created: " << (void*)display;
        fflush(stdout);

        if (display) {
            qDebug() << "[RobotModel] Calling subProp for Description Topic...";
            fflush(stdout);

            // 检查属性是否存在
            auto desc_prop = display->subProp("Description Topic");
            qDebug() << "[RobotModel] desc_prop = " << (void*)desc_prop;
            fflush(stdout);

            if (desc_prop) {
                qDebug() << "[RobotModel] Setting Description Topic value...";
                fflush(stdout);
                desc_prop->setValue(QString::fromStdString(robot_description_topic));
                qDebug() << "[RobotModel] Description Topic set";
            } else {
                qWarning() << "[RobotModel] Description Topic property not found!";
            }
            fflush(stdout);

            qDebug() << "[RobotModel] Setting other properties...";
            fflush(stdout);

            // 设置其他属性（不设置 Visual Enabled 等，可能不存在）

            qDebug() << "[RobotModel] Enabling display...";
            fflush(stdout);

            // 最后启用 display
            display->setEnabled(true);

            qDebug() << "RobotModel display added";
            fflush(stdout);
        } else {
            qWarning() << "[RobotModel] Failed to create display!";
        }
    } catch (const std::exception& e) {
        qWarning() << "Failed to add RobotModel:" << e.what();
    }
}

void RvizPanel::addGrid()
{
    if (!manager_) return;
    
    try {
        auto display = manager_->createDisplay(
            "rviz_default_plugins/Grid",
            "Grid",
            true);
        
        if (display) {
            display->subProp("Cell Size")->setValue(0.1);  // 10cm 格子
            display->subProp("Plane Cell Count")->setValue(20);
            display->subProp("Color")->setValue(QColor(160, 160, 160));
            display->subProp("Alpha")->setValue(0.5);
            
            qDebug() << "Grid display added";
        }
    } catch (const std::exception& e) {
        qWarning() << "Failed to add Grid:" << e.what();
    }
}

void RvizPanel::addTF()
{
    if (!manager_) return;
    
    try {
        auto display = manager_->createDisplay(
            "rviz_default_plugins/TF",
            "TF",
            true);
        
        if (display) {
            display->subProp("Show Names")->setValue(true);
            display->subProp("Show Axes")->setValue(true);
            display->subProp("Show Arrows")->setValue(false);
            display->subProp("Marker Scale")->setValue(0.1);
            
            qDebug() << "TF display added";
        }
    } catch (const std::exception& e) {
        qWarning() << "Failed to add TF:" << e.what();
    }
}

void RvizPanel::addAxes()
{
    if (!manager_) return;
    
    try {
        auto display = manager_->createDisplay(
            "rviz_default_plugins/Axes",
            "Axes",
            true);
        
        if (display) {
            display->subProp("Length")->setValue(0.5);
            display->subProp("Radius")->setValue(0.02);
            display->subProp("Reference Frame")->setValue("base_link");
            
            qDebug() << "Axes display added";
        }
    } catch (const std::exception& e) {
        qWarning() << "Failed to add Axes:" << e.what();
    }
}
