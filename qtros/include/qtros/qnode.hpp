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
#include <dbw_mkz_msgs/ThrottleInfoReport.h>


/*****************************************************************************
** Namespaces
*****************************************************************************/

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
        float throttle;
        //static const int a;
Q_SIGNALS:
	void loggingUpdated();
        void rosShutdown();

private:
	int init_argc;
	char** init_argv;
	ros::Publisher chatter_publisher;
        ros::Subscriber chatter_subscriber;
        ros::Subscriber throttle_subscriber;
        QStringListModel logging_model;

};

}  // namespace qtros

#endif /* qtros_QNODE_HPP_ */
