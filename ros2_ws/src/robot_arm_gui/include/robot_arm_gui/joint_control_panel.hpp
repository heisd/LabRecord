#ifndef ROBOT_ARM_GUI_JOINT_CONTROL_PANEL_HPP
#define ROBOT_ARM_GUI_JOINT_CONTROL_PANEL_HPP

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSlider>
#include <QLabel>
#include <QGroupBox>
#include <QPushButton>
#include <QDoubleSpinBox>
#include <vector>

class JointControlPanel : public QWidget
{
    Q_OBJECT

public:
    explicit JointControlPanel(int num_joints = 6, QWidget* parent = nullptr);
    ~JointControlPanel() override = default;

    // 设置关节角度（度）
    void setJointAngles(const std::vector<double>& angles_deg);
    
    // 获取当前角度（度）
    std::vector<double> getJointAngles() const;
    
    // 设置关节限位（度）
    void setJointLimits(int joint_index, double min_deg, double max_deg);
    
    // 设置关节名称
    void setJointName(int joint_index, const std::string& name);

signals:
    // 角度变化信号
    void jointAnglesChanged(const std::vector<double>& angles_deg);
    
    // 归零信号
    void goHomeRequested();
    
    // 准备位信号
    void goReadyRequested();

private slots:
    void onSliderChanged(int joint_index, int value);
    void onSpinBoxChanged(int joint_index, double value);

private:
    void setupUi();
    void emitAnglesChanged();

    int num_joints_;
    
    std::vector<QLabel*> name_labels_;
    std::vector<QSlider*> sliders_;
    std::vector<QDoubleSpinBox*> spinboxes_;
    std::vector<QLabel*> angle_labels_;
    
    // 关节限位
    std::vector<std::pair<double, double>> joint_limits_;
    
    // 防止循环更新
    bool updating_;
};

#endif // ROBOT_ARM_GUI_JOINT_CONTROL_PANEL_HPP
