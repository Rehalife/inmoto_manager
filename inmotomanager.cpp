#include "inmotomanager.h"
#include "ui_inmotomanager.h"

#include <QDebug>
#include <QMessageBox>
#include <QCloseEvent>

namespace fs = std::filesystem;

InMotoManager::InMotoManager(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::InMotoManager)
    , recording_folder_("RECORDINGS")
{
    ui->setupUi(this);

    // Stay Always on TOP
    auto flags = this->windowFlags();
    this->setWindowFlags(flags|Qt::WindowStaysOnTopHint);

    // ROS2 container process
    ros_start_process_ = new QProcess(this);
    ros_kill_process_ = new QProcess(this);

    // UI Connections
    connect(ui->deambulatore_on_btn, &QPushButton::clicked, this, &InMotoManager::on_deambulatore_on_btn_clicked);
    connect(ui->deambulatore_off_btn, &QPushButton::clicked, this, &InMotoManager::on_deambulatore_off_btn_clicked);

    // Power Led Widget
    ui->power_led->setShape(QLed::ledShape::Circle);
    ui->power_led->setColor(QColor("green"));
    ui->visualizer_power_led->setShape(QLed::ledShape::Circle);
    ui->visualizer_power_led->setColor(QColor("green"));

    // User Proximity Widgets
    ui->user_proximity_led->setShape(QLed::ledShape::Circle);
    ui->user_proximity_led->setColor(QColor("green"));
    ui->user_proximity_led->setOnColor(QColor("green"));
    ui->user_proximity_led->setOffColor(QColor("red"));

    // Default state
    ui->registrazione_esercizio_gbox->setEnabled(false);
    ui->user_proximity_gbox->setEnabled(false);
    ui->deambulatore_off_btn->setEnabled(false);
    ui->stop_exercise_btn->setEnabled(false);
    ui->stackedWidget->setCurrentIndex(0);

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
    on_deambulatore_off_btn_clicked(); // Turns off all processes if the user closes the app 'InMotoManager'

    //TODO chiamare anche on_visualizer_off_bnt_clicked();

    // Spegne ROS in modo pulito prima di distruggere il thread
    if(rclcpp::ok()){
     rclcpp::shutdown();
    }

    if(ros_thread_.joinable()){
     ros_thread_.join();
    }

    delete ui; // Release GUI memory
}

void InMotoManager::ROS_full_system_startup(){

    if (ros_start_process_->state() != QProcess::NotRunning) {
        qDebug() << "ROS 2 already active!";
        return;
    }

    QString prog = "bash";
    QStringList args;

    args << "-c" << "source /opt/ros/humble/setup.bash && source ~/inmoto_ws/install/setup.bash && ros2 launch inmoto_ros main_launch.py";

    qDebug() << "Starting ROS2...";
    ros_start_process_->start(prog, args);
}

void InMotoManager::ROS_visual_system_startup(){

    if (ros_start_process_->state() != QProcess::NotRunning) {
        qDebug() << "ROS 2 already active!";
        return;
    }

    QString prog = "bash";
    QStringList args;

    args << "-c" << "source /opt/ros/humble/setup.bash && source ~/inmoto_ws/install/setup.bash && ros2 launch inmoto_ros visualization_launch.py";

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

    bool ok{};
    QString recording_name{};

    while (recording_name.isEmpty()){

        recording_name = QInputDialog::getText(this, tr("Inizio registrazione"),
                                             tr("Nome registrazione:"), QLineEdit::Normal,
                                             "", &ok);
        if (!ok){
            QMessageBox::warning(this, "Registrazione annullata", "Operazione di registrazione annullata!");
            return;
        }

        if (recording_name.isEmpty()){
            QMessageBox::warning(this, "Informazioni mancanti", "Definire il nome della registrazione!");
        }
    }

    auto request = std::make_shared<inmoto_ros::srv::StartRecording::Request>();
    request->name = recording_folder_ + "/" + recording_name.toStdString();
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

void InMotoManager::closeEvent(QCloseEvent *event) {

    bool conditions_met = !ros_in_esecuzione_;

    if (!conditions_met) {

        QMessageBox::StandardButton answer;
        answer = QMessageBox::warning(this, "Conferma chiusura",
                                       "Prima di chiudere l'applicazione è necessario spegnere il sistema");

        event->ignore();
        return;
    }

    event->accept();
}


void InMotoManager::on_usa_deambulatore_btn_clicked()
{
    ui->stackedWidget->setCurrentIndex(1);
    ui->deambulatore_off_btn->setEnabled(false);

}

void InMotoManager::on_usa_visualizzatore_btn_clicked()
{
    ui->stackedWidget->setCurrentIndex(2);
    ui->visualizer_off_btn->setEnabled(false);
    ui->visualize_exercise_gbox->setEnabled(false);

}

void InMotoManager::on_deambulatore_on_btn_clicked() {

    // Update GUI and state
    ui->usa_visualizzatore_btn->setEnabled(false);
    ui->usa_deambulatore_btn->setEnabled(false);

    ui->registrazione_esercizio_gbox->setEnabled(true);
    ui->user_proximity_gbox->setEnabled(true);
    ui->deambulatore_off_btn->setEnabled(true);
    ui->deambulatore_on_btn->setEnabled(false);
    ros_in_esecuzione_ = true;

    ROS_full_system_startup();

    // Wait ROS2 starts completely..
    asyncWaitManagingGUI(5000, true);

    // Turn on power led
    ui->power_led->setValue(true);
}

void InMotoManager::on_deambulatore_off_btn_clicked() {

    if(registrazione_esercizio_in_corso_){
        QMessageBox::warning(this, "Chiusura sistema annullata", "Registrazione in corso: prima di spegnere il sistema bisogna fermare la registrazione.");
        return;
    }

    ROS_shutdown();

    // Wait ROS2 shuts down completely..
    asyncWaitManagingGUI(2000, true);

    // Turn off power led
    ui->power_led->setValue(false);

    // Update GUI
    ui->usa_visualizzatore_btn->setEnabled(true);
    ui->usa_deambulatore_btn->setEnabled(true);

    ui->registrazione_esercizio_gbox->setEnabled(false);
    ui->user_proximity_gbox->setEnabled(false);
    ui->deambulatore_off_btn->setEnabled(false);
    ui->deambulatore_on_btn->setEnabled(true);

    ros_in_esecuzione_ = false;

}


void InMotoManager::on_start_exercise_btn_clicked()
{
    // Update GUI and state
    ui->stop_exercise_btn->setEnabled(true);
    ui->start_exercise_btn->setEnabled(false);
    registrazione_esercizio_in_corso_ = true;

    ROS_clearTrajectory();
    syncWait(1000);

    ROS_startRecording();
    syncWait(1000);
}

void InMotoManager::on_stop_exercise_btn_clicked()
{
    ROS_stopRecording();
    syncWait(1000);

    ui->stop_exercise_btn->setEnabled(false);
    ui->start_exercise_btn->setEnabled(true);

    registrazione_esercizio_in_corso_ = false;
}

void InMotoManager::on_visualizer_on_btn_clicked()
{
    // Update GUI and state
    ui->usa_visualizzatore_btn->setEnabled(false);
    ui->usa_deambulatore_btn->setEnabled(false);
    ui->visualizer_on_btn->setEnabled(false);
    ui->visualizer_off_btn->setEnabled(true);
    ui->visualize_exercise_gbox->setEnabled(true);

    ROS_visual_system_startup();

    // Wait ROS2 starts completely..
    asyncWaitManagingGUI(2500, true);

    // Turn on power led
    ui->visualizer_power_led->setValue(true);
}

void InMotoManager::on_visualizer_off_btn_clicked()
{
    ROS_shutdown();

    // Wait ROS2 shuts down completely..
    asyncWaitManagingGUI(2000, true);

    // Turn off power led
    ui->visualizer_power_led->setValue(false);

    // Update GUI and state
    ui->usa_visualizzatore_btn->setEnabled(true);
    ui->usa_deambulatore_btn->setEnabled(true);
    ui->visualizer_on_btn->setEnabled(true);
    ui->visualizer_off_btn->setEnabled(false);
    ui->visualize_exercise_gbox->setEnabled(false);

}
