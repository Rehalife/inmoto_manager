#pragma once

#include <QMainWindow>
#include <QProcess>

#include <rclcpp/rclcpp.hpp>
#include <std_srvs/srv/trigger.hpp>
#include <std_msgs/msg/bool.hpp>


namespace Ui {
class InMotoManager;
}

class InMotoManager : public QMainWindow
{
    Q_OBJECT

public:
    InMotoManager(QWidget *parent = nullptr);
    ~InMotoManager();
    void waitManagingGUI(int msecs, bool disable_gui);

private:
    // ROS calls
    void ROS_startup();
    void ROS_shutdown();
    void ROS_clearTrajectory();

private:
    // ROS callbacks
    void user_proximity_topic_callback(std_msgs::msg::Bool msg);

private slots:
    void onStartButtonClicked();
    void onStopButtonClicked();
    void on_start_exercise_btn_clicked();

private:
    Ui::InMotoManager *ui;
    // Linux processes
    QProcess *ros_start_process_;
    QProcess *ros_kill_process_;
    QProcess *ros_start_bag_recording_;
    QProcess *ros_stop_bag_recording_;

    // ROS2 Interaction
    rclcpp::Node::SharedPtr node_;
    std::thread ros_thread_;
    rclcpp::Client<std_srvs::srv::Trigger>::SharedPtr clear_trajectory_client_;
    rclcpp::Subscription<std_msgs::msg::Bool>::SharedPtr user_proximity_subscriber_;

};
