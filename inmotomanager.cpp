#include "inmotomanager.h"
#include "ui_inmotomanager.h"
#include <QDebug>

namespace fs = std::filesystem;

InMotoManager::InMotoManager(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::InMotoManager)
{
    ui->setupUi(this);

    // ROS2 container process
    ros_start_process_ = new QProcess(this);
    ros_kill_process_ = new QProcess(this);

    // UI Connections
    connect(ui->on_btn, &QPushButton::clicked, this, &InMotoManager::onStartButtonClicked);
    connect(ui->off_btn, &QPushButton::clicked, this, &InMotoManager::onStopButtonClicked);

    // Power Led Widget
    ui->power_led->setShape(QLed::ledShape::Circle);
    ui->power_led->setColor(QColor("green"));

    // User Proximity Widgets
    ui->user_proximity_led->setShape(QLed::ledShape::Circle);
    ui->user_proximity_led->setColor(QColor("green"));
    ui->user_proximity_led->setOnColor(QColor("green"));
    ui->user_proximity_led->setOffColor(QColor("red"));

    // Default state
    ui->exercise_gbox->setEnabled(false);
    ui->user_proximity_gbox->setEnabled(false);


    // Setup ROS node
    node_ = rclcpp::Node::make_shared("inmoto_manager");

    // Setup ROS service clients
    clear_trajectory_client_ =
        node_->create_client<std_srvs::srv::Trigger>("/trajectory_publisher_node/clear_trajectory");
    start_recording_client_ =
            node_->create_client<inmoto_ros::srv::StartRecording>("/start_recording");
    stop_recording_client_ =
            node_->create_client<std_srvs::srv::Trigger>("/stop_recording");

    // Setup ROS Topic Subscriber
    user_proximity_subscriber_ = node_->create_subscription<std_msgs::msg::Bool>(
                "/user_proximity_close", 10, std::bind(&InMotoManager::user_proximity_topic_callback, this, std::placeholders::_1));

    // ROS Signals Thread
    ros_thread_ = std::thread([this]() {
        rclcpp::spin(node_);
    });
}

void InMotoManager::syncWait(int msec) {
    QEventLoop loop;
    // Quando il timer scatta, dice al loop di uscire
    QTimer::singleShot(msec, &loop, &QEventLoop::quit);
    // Avvia un sotto-loop di eventi: il codice si ferma qui, ma la GUI e i thread Qt continuano a girare!
    loop.exec();
}

void InMotoManager::asyncWaitManagingGUI(int msecs, bool disable_gui){
    if(disable_gui){
        this->setEnabled(false);
    }
    QTimer::singleShot(msecs, this, [this, disable_gui]() {
        if(disable_gui){
          this->setEnabled(true);
        }
    });
}

InMotoManager::~InMotoManager()
{
    onStopButtonClicked(); // Turns off all processes if the user closes the app 'InMotoManager'

    // Spegne ROS in modo pulito prima di distruggere il thread
    if(rclcpp::ok()){
     rclcpp::shutdown();
    }

    if(ros_thread_.joinable()){
     ros_thread_.join();
    }

    delete ui; // Release GUI memory
}

void InMotoManager::ROS_startup(){

    if (ros_start_process_->state() != QProcess::NotRunning) {
        qDebug() << "ROS 2 already active!";
        return;
    }

    //ros_process_->setCreateProcessGroup(true); // function non found in our qt system

    QString prog = "bash";
    QStringList args;

    args << "-c" << "source /opt/ros/humble/setup.bash && source ~/inmoto_ws/install/setup.bash && ros2 launch inmoto_ros main_launch.py";

    qDebug() << "Starting ROS2...";
    ros_start_process_->start(prog, args);
}

void InMotoManager::ROS_shutdown(){

    if (ros_start_process_->state() == QProcess::NotRunning) return;

    QString prog = "bash";
    QStringList args;

    args << "-c" << "source /opt/ros/humble/setup.bash && source ~/inmoto_ws/install/setup.bash && ros2 service call /inmoto/shutdowner/shutdown_system std_srvs/srv/Trigger {}";

    qDebug() << "Shutting down ROS 2...";
    ros_kill_process_->start(prog, args);

    qDebug() << "ROS 2 shut down successfully.";
}

void InMotoManager::ROS_clearTrajectory(){

    auto request = std::make_shared<std_srvs::srv::Trigger::Request>();
    callRosService(clear_trajectory_client_, request);
}

void InMotoManager::ROS_startRecording(){

    auto request = std::make_shared<inmoto_ros::srv::StartRecording::Request>();
    request->name = "esercizio";
    callRosService(start_recording_client_, request);
}

void InMotoManager::ROS_stopRecording(){

    auto request = std::make_shared<std_srvs::srv::Trigger::Request>();
    callRosService(stop_recording_client_, request);
}

void InMotoManager::user_proximity_topic_callback(std_msgs::msg::Bool msg) {
    ui->user_proximity_led->setValue(msg.data);

    if(msg.data){
        ui->user_proximity_label->setText("USER DETECTED");
    }else {
        ui->user_proximity_label->setText("! USER NOT DETECTED !");
    }
}

void InMotoManager::onStartButtonClicked() {

    ROS_startup();

    // Wait ROS2 starts completely..
    asyncWaitManagingGUI(5000, true);

    // Turn on power led
    ui->power_led->setValue(true);

    // Enable features
    ui->exercise_gbox->setEnabled(true);
    ui->user_proximity_gbox->setEnabled(true);
}

void InMotoManager::onStopButtonClicked() {

    ROS_shutdown();

    // Wait ROS2 shuts down completely..
    asyncWaitManagingGUI(2000, true);

    // Turn off power led
    ui->power_led->setValue(false);

    // Disable features
    ui->exercise_gbox->setEnabled(false);
    ui->user_proximity_gbox->setEnabled(false);

}


void InMotoManager::on_start_exercise_btn_clicked()
{
    ROS_clearTrajectory();
    syncWait(1000);

//    ROS_startRecording();
//    syncWait(1000);

}

void InMotoManager::on_stop_exercise_btn_clicked()
{
//    ROS_stopRecording();
//    syncWait(1000);
}
