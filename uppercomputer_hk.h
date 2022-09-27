#ifndef UPPERCOMPUTER_HK_H
#define UPPERCOMPUTER_HK_H

#include <QMainWindow>
#include <QtSerialPort/QSerialPort>
#include <QtSerialPort/QSerialPortInfo>
#include <QTimer>
#include <QMessageBox>
#include <cmath>
#include <cstring>
#include <QChartView>
#include <QChart>
#include <QValueAxis>
#include <QDateTimeAxis>
#include <QSplineSeries>
#include <QWidget>
#include <QSettings>
#include <QDebug>
#include <iostream>
using namespace std;
QT_BEGIN_NAMESPACE
namespace Ui { class UpperComputer_HK; }
QT_END_NAMESPACE
class UpperComputer_HK : public QMainWindow
{
    Q_OBJECT

public:
    UpperComputer_HK(QWidget *parent = nullptr);
    ~UpperComputer_HK();
    void reflashinfo();
    bool SerialportConfig();
    void UiConfig();
    void Chart_Config();
    void SettingsConfig();
    void SettingsSave();
    void Send();
    void AutoSend();
private slots:
    void on_Btn_OpenCom_clicked();
    void on_Btn_ClearRcvd_clicked();
    void on_Btn_ClearSend_clicked();
    void on_Btn_PIDMode_clicked();
    void on_Btn_Send_clicked();
    void on_Ckb_HexDisplay_stateChanged(int arg1);
    void on_Ckb_HexSend_stateChanged(int arg1);
    void on_Ckb_ChRcvd_stateChanged(int arg1);
    void on_Ckb_ChSend_stateChanged(int arg1);
    void on_Ckb_AutoSend_stateChanged(int arg1);

private:
    Ui::UpperComputer_HK *ui;
    QList<QSerialPortInfo> portinfo;    /*关于串口的信息*/
    QString Comname,Combaudrate,ComStopPos,ComDataPos,ComCheckPos;
    QSerialPort *myport;                /*串口*/
    QTimer *timerforport;               /*定时器*/
    QTimer *timerforsend;               /*自动发送的定时器*/
    QString SendBuff,RcvdBuff;          /*接收发送缓冲区*/
    bool SendMode;                      /*false--正常发送接收模式，true--PID发送接收模式*/
    QChart *PID_Chart;                  /*chart图表*/
    QSplineSeries *myLineSerials;       /*曲线*/
    QValueAxis *mAxX,*mAxY;             /*x，y轴*/
    QChartView *ChartWidget;            /*显示图表的窗口*/
    int old_Comnum,now_Comnum;          /*之前的串口数，现在的串口数*/
    QSettings *Configini;               /*读取串口配置信息的句柄*/
    double pid_set;                     /*pid的设定目标值*/
    int old_autotime,now_autotime;      /*自动发送的时间*/
    double x,y;                         /*点坐标*/
    QList<QPointF> Points;              /*所有的点*/
    QString curr_buff,curr_buff1;       /*创建单独的变量便于内存管理*/
    QByteArray byteCurr_Buff;           /*同上*/
    QPointF curr_point;                 /*同上*/
    QSerialPort::SerialPortError error; /*同上*/
    QString p,i,d;
};;
#endif // UPPERCOMPUTER_HK_H
