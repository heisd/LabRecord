#include "robot_arm_gui/joint_control_panel.hpp"

#include <QFrame>
#include <QFont>

JointControlPanel::JointControlPanel(int num_joints, QWidget* parent)
    : QWidget(parent)
    , num_joints_(num_joints)
    , updating_(false)
{
    // 初始化限位（默认 -180 到 180 度）
    joint_limits_.resize(num_joints_, {-180.0, 180.0});
    
    setupUi();
}

void JointControlPanel::setupUi()
{
    QVBoxLayout* main_layout = new QVBoxLayout(this);
    main_layout->setSpacing(10);
    
    // 标题
    QLabel* title = new QLabel("🦾 关节控制");
    QFont title_font = title->font();
    title_font.setPointSize(14);
    title_font.setBold(true);
    title->setFont(title_font);
    title->setAlignment(Qt::AlignCenter);
    main_layout->addWidget(title);
    
    // 分隔线
    QFrame* line = new QFrame();
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    main_layout->addWidget(line);
    
    // 关节控制组
    QGroupBox* joint_group = new QGroupBox("各轴角度");
    QVBoxLayout* joint_layout = new QVBoxLayout(joint_group);
    
    for (int i = 0; i < num_joints_; i++) {
        QHBoxLayout* row = new QHBoxLayout();
        row->setSpacing(8);
        
        // 关节名称
        QLabel* name_label = new QLabel(QString("J%1:").arg(i + 1));
        name_label->setFixedWidth(40);
        name_label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        name_labels_.push_back(name_label);
        row->addWidget(name_label);
        
        // 滑块
        QSlider* slider = new QSlider(Qt::Horizontal);
        slider->setMinimum(-180);
        slider->setMaximum(180);
        slider->setValue(0);
        slider->setTickPosition(QSlider::TicksBelow);
        slider->setTickInterval(45);
        sliders_.push_back(slider);
        row->addWidget(slider, 1);
        
        // 数值输入框
        QDoubleSpinBox* spinbox = new QDoubleSpinBox();
        spinbox->setRange(-180.0, 180.0);
        spinbox->setValue(0.0);
        spinbox->setSuffix("°");
        spinbox->setDecimals(1);
        spinbox->setFixedWidth(80);
        spinboxes_.push_back(spinbox);
        row->addWidget(spinbox);
        
        joint_layout->addLayout(row);
        
        // 连接信号 - 使用 lambda 捕获索引
        connect(slider, &QSlider::valueChanged, this,
            [this, i](int value) { onSliderChanged(i, value); });
        
        connect(spinbox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
            [this, i](double value) { onSpinBoxChanged(i, value); });
    }
    
    main_layout->addWidget(joint_group);
    
    // 快捷按钮组
    QGroupBox* action_group = new QGroupBox("快捷动作");
    QHBoxLayout* button_layout = new QHBoxLayout(action_group);
    
    QPushButton* home_btn = new QPushButton("🏠 归零");
    home_btn->setMinimumHeight(35);
    connect(home_btn, &QPushButton::clicked, this, &JointControlPanel::goHomeRequested);
    button_layout->addWidget(home_btn);
    
    QPushButton* ready_btn = new QPushButton("✋ 准备位");
    ready_btn->setMinimumHeight(35);
    connect(ready_btn, &QPushButton::clicked, this, &JointControlPanel::goReadyRequested);
    button_layout->addWidget(ready_btn);
    
    main_layout->addWidget(action_group);
    
    // 信息显示区
    QGroupBox* info_group = new QGroupBox("提示");
    QVBoxLayout* info_layout = new QVBoxLayout(info_group);
    
    QLabel* info_label = new QLabel(
        "• 拖动滑块或输入数值控制关节\n"
        "• 确保已加载机器人 URDF\n"
        "• 需要运行 robot_state_publisher");
    info_label->setWordWrap(true);
    info_label->setStyleSheet("color: #666;");
    info_layout->addWidget(info_label);
    
    main_layout->addWidget(info_group);
    
    // 弹性空间
    main_layout->addStretch();
}

void JointControlPanel::setJointAngles(const std::vector<double>& angles_deg)
{
    updating_ = true;
    
    for (size_t i = 0; i < angles_deg.size() && i < static_cast<size_t>(num_joints_); i++) {
        sliders_[i]->setValue(static_cast<int>(angles_deg[i]));
        spinboxes_[i]->setValue(angles_deg[i]);
    }
    
    updating_ = false;
    emitAnglesChanged();
}

std::vector<double> JointControlPanel::getJointAngles() const
{
    std::vector<double> angles;
    for (int i = 0; i < num_joints_; i++) {
        angles.push_back(spinboxes_[i]->value());
    }
    return angles;
}

void JointControlPanel::setJointLimits(int joint_index, double min_deg, double max_deg)
{
    if (joint_index >= 0 && joint_index < num_joints_) {
        joint_limits_[joint_index] = {min_deg, max_deg};
        sliders_[joint_index]->setMinimum(static_cast<int>(min_deg));
        sliders_[joint_index]->setMaximum(static_cast<int>(max_deg));
        spinboxes_[joint_index]->setRange(min_deg, max_deg);
    }
}

void JointControlPanel::setJointName(int joint_index, const std::string& name)
{
    if (joint_index >= 0 && joint_index < num_joints_) {
        name_labels_[joint_index]->setText(QString::fromStdString(name) + ":");
        name_labels_[joint_index]->setToolTip(QString::fromStdString(name));
    }
}

void JointControlPanel::onSliderChanged(int joint_index, int value)
{
    if (updating_) return;
    
    updating_ = true;
    spinboxes_[joint_index]->setValue(static_cast<double>(value));
    updating_ = false;
    
    emitAnglesChanged();
}

void JointControlPanel::onSpinBoxChanged(int joint_index, double value)
{
    if (updating_) return;
    
    updating_ = true;
    sliders_[joint_index]->setValue(static_cast<int>(value));
    updating_ = false;
    
    emitAnglesChanged();
}

void JointControlPanel::emitAnglesChanged()
{
    if (updating_) return;
    emit jointAnglesChanged(getJointAngles());
}
