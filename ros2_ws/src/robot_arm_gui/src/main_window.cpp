#include "robot_arm_gui/main_window.hpp"
#include "robot_arm_gui/rviz_panel.hpp"
#include "robot_arm_gui/joint_control_panel.hpp"

#include <QSplitter>
#include <QStatusBar>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QMessageBox>
#include <cmath>

MainWindow::MainWindow(rclcpp::Node::SharedPtr node, QWidget* parent)
    : QMainWindow(parent)
    , node_(node)
    , rviz_panel_(nullptr)
    , joint_panel_(nullptr)
{
    // 初始化关节数据
    joint_names_ = {"joint_1", "joint_2", "joint_3", "joint_4", "joint_5", "joint_6"};
    current_joint_angles_.resize(6, 0.0);
    
    // 创建发布者
    joint_pub_ = node_->create_publisher<sensor_msgs::msg::JointState>(
        "/joint_states", 10);
    
    // 定时发布关节状态（20Hz）
    timer_ = node_->create_wall_timer(
        std::chrono::milliseconds(50),
        std::bind(&MainWindow::publishJointStates, this));
    
    setupUi();
}

MainWindow::~MainWindow()
{
    if (timer_) {
        timer_->cancel();
    }
}

void MainWindow::setupUi()
{
    // 中心部件
    QWidget* central = new QWidget(this);
    setCentralWidget(central);
    
    QHBoxLayout* main_layout = new QHBoxLayout(central);
    
    // 使用 QSplitter 可以拖动调整大小
    QSplitter* splitter = new QSplitter(Qt::Horizontal, central);
    
    // 左侧：RViz 面板
    rviz_panel_ = new RvizPanel(node_, splitter);
    rviz_panel_->setMinimumWidth(600);
    splitter->addWidget(rviz_panel_);
    
    // 右侧：控制面板
    joint_panel_ = new JointControlPanel(6, splitter);
    joint_panel_->setMinimumWidth(250);
    joint_panel_->setMaximumWidth(400);
    
    // 设置关节名称和限位
    std::vector<std::pair<double, double>> limits = {
        {-180, 180},  // Joint 1: 底座
        {-90, 90},    // Joint 2: 肩部
        {-135, 135},  // Joint 3: 肘部
        {-180, 180},  // Joint 4: 腕部旋转
        {-90, 90},    // Joint 5: 腕部俯仰
        {-180, 180}   // Joint 6: 末端
    };
    
    for (int i = 0; i < 6; i++) {
        joint_panel_->setJointName(i, joint_names_[i]);
        joint_panel_->setJointLimits(i, limits[i].first, limits[i].second);
    }
    
    splitter->addWidget(joint_panel_);
    
    // 设置初始比例
    splitter->setStretchFactor(0, 3);  // RViz 占 3
    splitter->setStretchFactor(1, 1);  // 控制面板占 1
    
    main_layout->addWidget(splitter);
    
    // 连接信号
    connect(joint_panel_, &JointControlPanel::jointAnglesChanged,
            this, &MainWindow::onJointAnglesChanged);
    connect(joint_panel_, &JointControlPanel::goHomeRequested,
            this, &MainWindow::onGoHome);
    connect(joint_panel_, &JointControlPanel::goReadyRequested,
            this, &MainWindow::onGoReady);
    
    // 菜单栏
    QMenu* file_menu = menuBar()->addMenu("文件(&F)");
    QAction* quit_action = file_menu->addAction("退出(&Q)");
    connect(quit_action, &QAction::triggered, this, &QMainWindow::close);
    
    QMenu* view_menu = menuBar()->addMenu("视图(&V)");
    QAction* reset_view = view_menu->addAction("重置视角(&R)");
    connect(reset_view, &QAction::triggered, this, [this]() {
        QMessageBox::information(this, "提示", "视角已重置");
        // TODO: 实现真正的视角重置
    });
    
    // 状态栏
    statusBar()->showMessage("就绪 - 等待 robot_description...");
}

void MainWindow::initializeRviz()
{
    if (rviz_panel_) {
        rviz_panel_->initialize();
        rviz_panel_->addGrid();
        rviz_panel_->addRobotModel();
        rviz_panel_->addTF();
        
        statusBar()->showMessage("RViz 初始化完成");
    }
}

void MainWindow::onJointAnglesChanged(const std::vector<double>& angles_deg)
{
    // 度转弧度
    for (size_t i = 0; i < angles_deg.size() && i < current_joint_angles_.size(); i++) {
        current_joint_angles_[i] = angles_deg[i] * M_PI / 180.0;
    }
    
    // 更新状态栏
    QString status = "关节角度: ";
    for (size_t i = 0; i < angles_deg.size(); i++) {
        status += QString("J%1=%2° ").arg(i+1).arg(angles_deg[i], 0, 'f', 1);
    }
    statusBar()->showMessage(status);
}

void MainWindow::onGoHome()
{
    std::vector<double> home(6, 0.0);
    joint_panel_->setJointAngles(home);
}

void MainWindow::onGoReady()
{
    std::vector<double> ready = {0, -45, 90, 0, 45, 0};
    joint_panel_->setJointAngles(ready);
}

void MainWindow::publishJointStates()
{
    auto msg = sensor_msgs::msg::JointState();
    msg.header.stamp = node_->get_clock()->now();
    msg.name = joint_names_;
    msg.position = current_joint_angles_;
    
    joint_pub_->publish(msg);
}
