/**
 * @file /include/qtros/qnode.hpp
 *
 * @brief Communications central!
 *
 * @date February 2011
 **/
/*****************************************************************************
** Ifdefs
*****************************************************************************/

#ifndef qtros_QNODE_HPP_
#define qtros_QNODE_HPP_

/*****************************************************************************
** Includes
*****************************************************************************/

// To workaround boost/qt4 problems that won't be bugfixed. Refer to
//    https://bugreports.qt.io/browse/QTBUG-22829

#ifndef Q_MOC_RUN
#include <ros/ros.h>
#endif
#include <string>

#include <QThread>
#include <QStringListModel>
#include <std_msgs/Float64.h>

//Autoware msg
#include <dbw_mkz_msgs/ThrottleInfoReport.h>
#include <dbw_mkz_msgs/SteeringReport.h>
#include <dbw_mkz_msgs/GearReport.h>
#include <dbw_mkz_msgs/BrakeReport.h>
#include <dbw_mkz_msgs/FuelLevelReport.h>
#include <dbw_mkz_msgs/TurnSignalCmd.h>
#include <dbw_mkz_msgs/WheelSpeedReport.h>

#include "ui_main_window.h"


/*****************************************************************************
** Namespaces
*****************************************************************************/
using namespace std;

namespace qtros {

/*****************************************************************************
** Class
*****************************************************************************/

class QNode : public QThread {
    Q_OBJECT
public:
	QNode(int argc, char** argv );
	virtual ~QNode();
	bool init();
	bool init(const std::string &master_url, const std::string &host_url);
	void run();
        void myCallback(const std_msgs::Float64& message_holder);
        void throttle_Callback(const dbw_mkz_msgs::ThrottleInfoReportConstPtr &message_holder);
        void steeringreport_Callback(const dbw_mkz_msgs::SteeringReportPtr &message_holder);
        void gear_Callback(const dbw_mkz_msgs::GearReportPtr &message_holder);
        void brake_Callback(const dbw_mkz_msgs::BrakeReportPtr &message_holder);
        void fuel_Callback(const dbw_mkz_msgs::FuelLevelReportPtr &fuel_holder);
        void turn_signal_Callback(const dbw_mkz_msgs::TurnSignalCmdPtr &turn_signal_holder);
        void wheel_speed_Callback(const dbw_mkz_msgs::WheelSpeedReportPtr &wheel_speed_holder);
        void control_mode_Callback(const dbw_mkz_msgs::WheelSpeedReportPtr &wheel_speed_holder);






        /*********************
	** Logging
	**********************/
	enum LogLevel {
	         Debug,
	         Info,
	         Warn,
	         Error,
	         Fatal
	 };

	QStringListModel* loggingModel() { return &logging_model; }
        void log( const LogLevel &level, const std_msgs::Float64 &msg);
        //void log1( const LogLevel &level, const std_msgs::Float64 &msg);


         //車體資訊
         int throttle;
         float steering_angle;
         float vehicle_speed;
         string gear;
         char* test1;
         float brake;
         float fuel_level;
         string turn_signal;
         float wheel_speed;
         string control_mode;

Q_SIGNALS:
	void loggingUpdated();
        void rosShutdown();

private:
	int init_argc;
	char** init_argv;
	ros::Publisher chatter_publisher;
        ros::Subscriber chatter_subscriber;
        ros::Subscriber throttle_subscriber;
        ros::Subscriber steeringreport_subscriber;
        ros::Subscriber gear_subscriber;
        ros::Subscriber brake_subscriber;
        ros::Subscriber fuel_subscriber;
        ros::Subscriber turn_signal_subscriber;
        ros::Subscriber wheel_speed_subscriber;
        ros::Subscriber control_mode_subscriber;


        QStringListModel logging_model;

};

}  // namespace qtros

#endif /* qtros_QNODE_HPP_ */
