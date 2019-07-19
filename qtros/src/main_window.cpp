/**
 * @file /src/main_window.cpp
 *
 * @brief Implementation for the qt gui.
 *
 * @date February 2011
 **/
/*****************************************************************************
** Includes
*****************************************************************************/

#include <QtGui>
#include <QMessageBox>
#include <iostream>
#include "../include/qtros/main_window.hpp"

/*****************************************************************************
** Namespaces
*****************************************************************************/

namespace qtros {

using namespace Qt;

/*****************************************************************************
** Implementation [MainWindow]
*****************************************************************************/




//QObject::connect(ui.actionAbout_Qt, SIGNAL(triggered(bool)), qApp, SLOT(aboutQt())); // qApp is a global variable for the application


//QObject::connect(&qnode, SIGNAL(rosShutdown()), this, SLOT(close()));////


//QObject::connect(&qnode, SIGNAL(loggingUpdated()), this, SLOT(updateLoggingView()));

MainWindow::MainWindow(int argc, char** argv, QWidget *parent)
	: QMainWindow(parent)
	, qnode(argc,argv)
{
	ui.setupUi(this); // Calling this incidentally connects all ui's triggers to on_...() callbacks in this class.
    QObject::connect(ui.actionAbout_Qt, SIGNAL(triggered(bool)), qApp, SLOT(aboutQt())); // qApp is a global variable for the application

    ReadSettings();
	setWindowIcon(QIcon(":/images/icon.png"));
	ui.tab_manager->setCurrentIndex(0); // ensure the first tab is showing - qt-designer should have this already hardwired, but often loses it (settings?).
    QObject::connect(&qnode, SIGNAL(rosShutdown()), this, SLOT(close()));

	/*********************
	** Logging
	**********************/
    ui.view_logging->setModel(qnode.loggingModel());
    QObject::connect(&qnode, SIGNAL(loggingUpdated()), this, SLOT(updateLoggingView()));


    /*********************
    ** Auto Start
    **********************/
    if ( ui.checkbox_remember_settings->isChecked() ) {
        on_button_connect_clicked(true);
    }

   //QObject::connect(ui.pushButton, SIGNAL(toggled(bool)), qApp, SLOT(refreshUi()));
   // QObject::connect(ui.pushButton1, SIGNAL(toggled(bool)), this, SLOT(moveLeft()));
   //QObject::connect(ui.pushButton1, SIGNAL(clicked()), this, SLOT(moveLeft()));
    QObject::connect(ui.pushButton1, SIGNAL(clicked()), this, SLOT( moveLeft()));



}

MainWindow::~MainWindow() {}

/*****************************************************************************
** Implementation [Slots]
*****************************************************************************/
void MainWindow::refreshUi() {
    ui.progressBar->setValue(qnode.throttle);
}

void MainWindow::showNoMasterMessage() {
	QMessageBox msgBox;
	msgBox.setText("Couldn't find the ros master.");
	msgBox.exec();
    close();
}

/*
 * These triggers whenever the button is clicked, regardless of whether it
 * is already checked or not.
 */
void MainWindow::moveLeft() {

    system("gnome-terminal -x bash -c  'cd ~/Desktop/ROS_KAFKA/kafka_2.11-1.1.0/bin;  ./kafka-server-start.sh ../config/server.properties'");
}

///////////////////////////////////////////////////////////////////////////////////////////
//connect
void MainWindow::on_button_connect_clicked(bool check ) {
	if ( ui.checkbox_use_environment->isChecked() ) {
		if ( !qnode.init() ) {
			showNoMasterMessage();
		} else {
			ui.button_connect->setEnabled(false);
		}
	} else {
		if ( ! qnode.init(ui.line_edit_master->text().toStdString(),
				   ui.line_edit_host->text().toStdString()) ) {
			showNoMasterMessage();
		} else {
            ui.button_connect->setEnabled(false);
			ui.line_edit_master->setReadOnly(true);
			ui.line_edit_host->setReadOnly(true);
			ui.line_edit_topic->setReadOnly(true);
		}
	}
    //ui.progressBar->setValue(qnode.throttle);
}



void MainWindow::on_checkbox_use_environment_stateChanged(int state) {
    bool enabled;
    if ( state == 0 ) {
        enabled = true;
    } else {
        enabled = false;
    }
    ui.line_edit_master->setEnabled(enabled);
    ui.line_edit_host->setEnabled(enabled);
    //ui.line_edit_topic->setEnabled(enabled);
}
///////////////////////////////////////////////////////////////////////////////////////////
//rviz
void MainWindow::showButtonTestMessage() {

    QMessageBox msgBox;
    msgBox.setText("Open rviz");
    system("gnome-terminal -x bash -c  'source ~/catkin_ws/devel/setup.bash;  rviz'");
    msgBox.exec();

    //close();
}


//void MainWindow::on_pushButton_pressed(){


//    ui.progressBar->setValue(qnode.throttle);

//    if(!timer1)
//        {
//            timer1 = new QTimer();
//            connect(timer1,&QTimer::timeout,this,&MainWindow::on_pushButton_clicked);
//        }
//        timer1->start(100);

//}


//void MainWindow::on_pushButton_released(){


//      timer1->stop();
//}

//顯示車體資訊
void MainWindow::on_pushButton_clicked(bool check){

    logging_model = qnode.loggingModel();
    logging_model->insertRows(logging_model->rowCount(), 1);
    std::stringstream logging_model_msg;
    logging_model_msg << qnode.steering_angle;
    QVariant new_row(QString(logging_model_msg.str().c_str()));
    logging_model->setData(logging_model->index(logging_model->rowCount()-1), new_row);
    std::cout << logging_model_msg.str().c_str() << std::endl;


    //progressBar
    ui.progressBar->setValue(qnode.throttle);
    ui.progressBar_2->setValue(qnode.fuel_level);


    //lcdNumber
    ui.lcdNumber->display(qnode.steering_angle);
    ui.lcdNumber_2->display(qnode.vehicle_speed);
    ui.lcdNumber_3->display(qnode.brake);
    ui.lcdNumber_4->display(qnode.wheel_speed);

    //textBrowser
    const char* c = qnode.gear.c_str();//qnode.gear.c_str();
    ui.textBrowser->setText(c);
    const char* d = qnode.turn_signal.c_str();
    ui.textBrowser_11->setText(d);





    //ui.pushButton->setAutoRepeat(true);

}

//kafka-1
void MainWindow::on_button_test_clicked(bool check) {


//ui.view_logging->scrollToBottom();

//   logging_model = qnode.loggingModel();
//   logging_model->insertRows(logging_model->rowCount(), 1);
//   std::stringstream logging_model_msg;
//   logging_model_msg << "text on the list";
//   QVariant new_row(QString(logging_model_msg.str().c_str()));
//   logging_model->setData(logging_model->index(logging_model->rowCount()-1), new_row);

//   std::cout << logging_model->rowCount() << std::endl;
//   std::cout << logging_model_msg.str().c_str() << std::endl;
//    QMessageBox msgBox;
//    msgBox.setText("Open kafka");
    system("gnome-terminal -x bash -c  'cd ~/Desktop/ROS_KAFKA/kafka_2.11-1.1.0/bin; ./zookeeper-server-start.sh ../config/zookeeper.properties'");
//system("gnome-terminal -x bash -c  'cd ~/Desktop/ROS_KAFKA/kafka_2.11-1.1.0/bin;  ./kafka-server-start.sh ../config/server.properties'");
//    msgBox.exec();


}

//button subscribe
void MainWindow::on_pushButton1_clicked(bool check) {
    QMessageBox msgBox;
    msgBox.setText("Open rviz");
     //showButtonTestMessage();

}

void MainWindow::on_pushButton_3_clicked(bool check){
    system("gnome-terminal -x bash -c 'export ROS_MASTER_URI=http://192.168.43.202:11311; export ROS_IP=192.168.43.219; roslaunch kafka_ros producer.launch'");
}

void MainWindow::on_pushButton_4_clicked(bool check){
    system("gnome-terminal -x bash -c 'export ROS_MASTER_URI=http://192.168.43.219:11311; export ROS_IP=192.168.43.219; roslaunch kafka_ros consumer.launch'");
}

void MainWindow::on_pushButton_7_clicked(bool check){
    system("gnome-terminal -x bash -c 'export ROS_MASTER_URI=http://192.168.43.219:11311; export ROS_IP=192.168.43.219; cd ~/Autoware/ros; ./run_no_roscore'");
}

void MainWindow::on_pushButton_5_clicked(bool check){
    system("gnome-terminal -x bash -c 'docker run -p 8080:8080 -d -t -v ~/mapproxy:/mapproxy danielsnider/mapproxy'");
    system("gnome-terminal -x bash -c 'export ROS_MASTER_URI=http://192.168.43.202:11311; export ROS_IP=192.168.43.219;roslaunch rviz_satellite shalun_image.launch'");
}

void MainWindow::on_pushButton_6_clicked(bool check){
    system("gnome-terminal -x bash -c 'export ROS_MASTER_URI=http://192.168.43.202:11311; export ROS_IP=192.168.43.219; roslaunch qtros rm.launch'");
}





/*****************************************************************************
** Implemenation [Slots][manually connected]
*****************************************************************************/

/**
 * This function is signalled by the underlying model. When the model changes,
 * this will drop the cursor down to the last line in the QListview to ensure
 * the user can always see the latest log message.
 */
void MainWindow::updateLoggingView() {
        ui.view_logging->scrollToBottom();
}

/*****************************************************************************
** Implementation [Menu]
*****************************************************************************/

void MainWindow::on_actionAbout_triggered() {
    QMessageBox::about(this, tr("About ..."),tr("<h2>PACKAGE_NAME Test Program 0.10</h2><p>Copyright Yujin Robot</p><p>This package needs an about description.</p>"));
}

/*****************************************************************************
** Implementation [Configuration]
*****************************************************************************/

void MainWindow::ReadSettings() {
    QSettings settings("Qt-Ros Package", "qtros");
    restoreGeometry(settings.value("geometry").toByteArray());
    restoreState(settings.value("windowState").toByteArray());
    QString master_url = settings.value("master_url",QString("http://meclab:11311/")).toString();
    QString host_url = settings.value("host_url", QString("192.168.0.112")).toString();
    //QString topic_name = settings.value("topic_name", QString("/chatter")).toString();
    ui.line_edit_master->setText(master_url);
    ui.line_edit_host->setText(host_url);
    //ui.line_edit_topic->setText(topic_name);
    bool remember = settings.value("remember_settings", false).toBool();
    ui.checkbox_remember_settings->setChecked(remember);
    bool checked = settings.value("use_environment_variables", false).toBool();
    ui.checkbox_use_environment->setChecked(checked);
    if ( checked ) {
    	ui.line_edit_master->setEnabled(false);
    	ui.line_edit_host->setEnabled(false);
    	//ui.line_edit_topic->setEnabled(false);
    }
}

void MainWindow::WriteSettings() {
    QSettings settings("Qt-Ros Package", "qtros");
    settings.setValue("master_url",ui.line_edit_master->text());
    settings.setValue("host_url",ui.line_edit_host->text());
    settings.setValue("topic_name",ui.line_edit_topic->text());
    settings.setValue("use_environment_variables",QVariant(ui.checkbox_use_environment->isChecked()));
    settings.setValue("geometry", saveGeometry());
    settings.setValue("windowState", saveState());
    settings.setValue("remember_settings",QVariant(ui.checkbox_remember_settings->isChecked()));

}

void MainWindow::closeEvent(QCloseEvent *event)
{
	WriteSettings();
	QMainWindow::closeEvent(event);
}

}  // namespace qtros
