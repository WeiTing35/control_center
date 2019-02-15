/**
 * @file /include/qtros/main_window.hpp
 *
 * @brief Qt based gui for qtros.
 *
 * @date November 2010
 **/
#ifndef qtros_MAIN_WINDOW_H
#define qtros_MAIN_WINDOW_H

/*****************************************************************************
** Includes
*****************************************************************************/

#include <QtGui/QMainWindow>
#include "ui_main_window.h"
#include "qnode.hpp"
#include "widget.h"


/*****************************************************************************
** Namespace
*****************************************************************************/

namespace qtros {

/*****************************************************************************
** Interface [MainWindow]
*****************************************************************************/
/**
 * @brief Qt central, all operations relating to the view part here.
 */
class MainWindow : public QMainWindow {
Q_OBJECT

public:
	MainWindow(int argc, char** argv, QWidget *parent = 0);
	~MainWindow();

	void ReadSettings(); // Load up qt program settings at startup
	void WriteSettings(); // Save qt program settings when closing

	void closeEvent(QCloseEvent *event); // Overloaded function
	void showNoMasterMessage();
        void showButtonTestMessage();
        void refreshUi();

        //float throttle;

public Q_SLOTS:
	/******************************************
	** Auto-connections (connectSlotsByName())
	*******************************************/
	void on_actionAbout_triggered();
	void on_button_connect_clicked(bool check );
	void on_checkbox_use_environment_stateChanged(int state);
        void on_button_test_clicked(bool check);
        void on_pushButton1_clicked(bool check) ;
        void moveLeft();
        void on_pushButton_3_clicked(bool check) ;
        void on_pushButton_4_clicked(bool check) ;
        void on_pushButton_7_clicked(bool check) ;
        void on_pushButton_5_clicked(bool check) ;
        void on_pushButton_6_clicked(bool check) ;


        //顯示車體資訊
        //void on_pushButton_pressed() ;
        //void on_pushButton_released() ;
        void on_pushButton_clicked(bool check) ;
    /******************************************
    ** Manual connections
    *******************************************/
    void updateLoggingView(); // no idea why this can't connect automatically

private:
	Ui::MainWindowDesign ui;
        QStringListModel* logging_model;
	QNode qnode;
        QLCDNumber *myLcd;
};

}  // namespace qtros

#endif // qtros_MAIN_WINDOW_H
