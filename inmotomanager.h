#pragma once

#include <QMainWindow>
#include <QProcess>

#include <rclcpp/rclcpp.hpp>
#include <std_srvs/srv/trigger.hpp>
#include <std_msgs/msg/bool.hpp>
#include <inmoto_ros/srv/start_recording.hpp>


namespace Ui {
class InMotoManager;
}

class InMotoManager : public QMainWindow
{
    Q_OBJECT

public:
    InMotoManager(QWidget *parent = nullptr);
    ~InMotoManager();
    void asyncWaitManagingGUI(int msecs, bool disable_gui);
    void syncWait(int msec);

private:

    template<typename CLIENT, typename REQUEST>
    void callRosService(CLIENT client, REQUEST request){

        auto future = client->async_send_request(request);

        if (future.wait_for(std::chrono::seconds(2)) == std::future_status::ready)
        {
            auto response = future.get();
        }
        else
        {
            // Timeout
            client->remove_pending_request(future);
            RCLCPP_ERROR(node_->get_logger(), "Service call timed out");
        }
    }

private:
    // ROS calls
    void ROS_startup();
    void ROS_shutdown();
    void ROS_clearTrajectory();
    void ROS_startRecording();
    void ROS_stopRecording();

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    // ROS callbacks
    void user_proximity_topic_callback(std_msgs::msg::Bool msg);

private slots:
    void onStartButtonClicked();
    void onStopButtonClicked();
    void on_start_exercise_btn_clicked();
    void on_stop_exercise_btn_clicked();

private:
    Ui::InMotoManager *ui;
    // Linux processes
    QProcess *ros_start_process_;
    QProcess *ros_kill_process_;

    // ROS2 Interaction
    rclcpp::Node::SharedPtr node_;
    std::thread ros_thread_;
    rclcpp::Client<std_srvs::srv::Trigger>::SharedPtr clear_trajectory_client_;
    rclcpp::Client<inmoto_ros::srv::StartRecording>::SharedPtr start_recording_client_;
    rclcpp::Client<std_srvs::srv::Trigger>::SharedPtr stop_recording_client_;
    rclcpp::Subscription<std_msgs::msg::Bool>::SharedPtr user_proximity_subscriber_;

    // Stato
    bool ros_in_esecuzione_ = false;
    bool registrazione_esercizio_in_corso_ = false;

};
