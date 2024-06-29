#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSerialPort>
#include <QSerialPortInfo>

#include <QList>
#include <QMessageBox>
#include <QDateTime>
#include <QWebChannel>
#include "bridge.h"
// 引入串口通信的两个头文件(第一步)
#include <QtSerialPort/QSerialPort>         // 提供访问串口的功能
#include <QtSerialPort/QSerialPortInfo>     // 提供系统中存在的串口信息


#include <QList>
#include <QMessageBox>
#include <QDateTime>
#pragma execution_character_set("utf-8")
namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

protected://串口
    void findFreePorts();
    bool initSerialPort();
    void sendMsg(const QString &msg);

private slots:
    void DisplaySlot(QString lng,QString lat);
    void on_pushButton_clicked();
    //串口
    void recvMsg();
    void handleCoordinates1(double j, double w);

private:
    Ui::MainWindow *ui;
    bridge *JSBridge;
    //串口
    QSerialPort *serialPort;
    //经纬度
    double longitude;   // 经度
    double latitude;    // 纬度

    // 在头文件中定义一个成员变量来存储接收到的数据
    QByteArray receivedDataBuffer;
};

#endif // MAINWINDOW_H
