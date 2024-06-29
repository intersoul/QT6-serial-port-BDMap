#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    /**********************************/
    this->serialPort = new QSerialPort;
    findFreePorts();

    connect(ui->openCom, &QCheckBox::toggled, [=](bool checked){
        if (checked){
            initSerialPort();
            ui->btnSend->setEnabled(true);
        }else{
            this->serialPort->close();
            ui->btnSend->setEnabled(false);
            ui->openCom->setChecked(false);
        }
    });

    connect(this->serialPort, SIGNAL(readyRead()), this, SLOT(recvMsg()));
    connect(ui->btnSend, &QPushButton::clicked, [=](){
        sendMsg(ui->message->toPlainText());
    });
    /******************************************/

    JSBridge = new bridge(this);
    QWebChannel *channel = new QWebChannel(this);
    channel->registerObject("window",(QObject*)JSBridge);
    ui->MapWidget->page()->setWebChannel(channel);
    ui->MapWidget->page()->load(QUrl("qrc:/Baidu_JS/BDMap.html"));
    connect(JSBridge,SIGNAL(DisplayPoint(QString,QString)),this,SLOT(DisplaySlot(QString,QString)));
    setWindowTitle("GPS定位系统");
}

MainWindow::~MainWindow()
{
    delete ui;
}

/***************************************************************/
void MainWindow::DisplaySlot(QString lng, QString lat)
{
    ui->lineEdit_RcvMsg->setText(lng+","+lat);
}

void MainWindow::on_pushButton_clicked()
{
    QString lng = QString::number(longitude);
    QString lat = QString::number(latitude);

    ui->MapWidget->page()->runJavaScript(QString("SetPoint(%1,%2)").arg(lng).arg(lat));
}

//void MainWindow::on_pushButton_clicked()
//{

//    QString context = ui->lineEdit_SendMsg->text();
//    if(!context.contains(','))
//    {
//        qDebug()<<"输入格式错误";        //输入格式 经度+纬度，中间以英文逗号‘,’隔开
//        return;
//    }
//    QString lng = context.split(',').at(0);
//    QString lat = context.split(',').at(1);

//    ui->MapWidget->page()->runJavaScript(QString("SetPoint(%1,%2)").arg(lng).arg(lat));
//}
/****************************************************************/

//寻找空闲状态串口
void MainWindow::findFreePorts()
{
    QList<QSerialPortInfo> ports = QSerialPortInfo::availablePorts();
    for (int i = 0; i < ports.size(); ++i)
    {
        if (ports.at(i).isBusy())
        {
            ports.removeAt(i);
            continue;
        }
        ui->portName->addItem(ports.at(i).portName());
    }
    if (!ports.size())
    {
        QMessageBox::warning(NULL,"Tip",QStringLiteral("/*找不到空闲串口*/"));
        return;
    }
}
//初始化串口
bool MainWindow::initSerialPort()
{
    this->serialPort->setPortName(ui->portName->currentText());
    if (!this->serialPort->open(QIODevice::ReadWrite))
    {
        QMessageBox::warning(NULL,"Tip",QStringLiteral("串口打开失败"));
        return false;
    }
    this->serialPort->setBaudRate(ui->baudRate->currentText().toInt());

    if (ui->dataBits->currentText().toInt() == 8)
    {
        this->serialPort->setDataBits(QSerialPort::Data8);
    }else if (ui->dataBits->currentText().toInt() == 7)
    {
        this->serialPort->setDataBits(QSerialPort::Data7);
    }else if (ui->dataBits->currentText().toInt() == 6)
    {
        this->serialPort->setDataBits(QSerialPort::Data6);
    }else if (ui->dataBits->currentText().toInt() == 5)
    {
        this->serialPort->setDataBits(QSerialPort::Data5);
    }

    if (ui->stopBits->currentText().toInt() == 1)
    {
        this->serialPort->setStopBits(QSerialPort::OneStop);
    }else if (ui->stopBits->currentText().toInt() == 2)
    {
        this->serialPort->setStopBits(QSerialPort::TwoStop);
    }


    if(ui->parity->currentText() == "NoParity")
    {
        this->serialPort->setParity(QSerialPort::NoParity);
    }else if (ui->parity->currentText() == "EvenParity"){
        this->serialPort->setParity(QSerialPort::EvenParity);
    }else if (ui->parity->currentText() == "OddParity"){
        this->serialPort->setParity(QSerialPort::OddParity);
    }
    return true;
}
//向串口发送信息
void MainWindow::sendMsg(const QString &msg)
{
    this->serialPort->write(QByteArray::fromHex(msg.toLatin1()));
    ui->comLog->insertPlainText(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") + " [send] " + msg + "\n");
}
//接受来自串口的信息
//void MainWindow::recvMsg()
//{
//    QByteArray msg = this->serialPort->readAll();
//    //do something
//    ui->comLog->insertPlainText(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") + " [recieve] " + msg.toHex().data() + "\n");
//}

void MainWindow::handleCoordinates1(double j, double w)
{
    // 将经纬度存储到成员变量中
    longitude = j;
    latitude = w;

    // 调用按钮点击事件函数
    on_pushButton_clicked();
}


void MainWindow::recvMsg()
{
    // 读取串口缓冲区中所有的数据
    QByteArray msg = this->serialPort->readAll();

    // 将数据添加到缓冲区中
    receivedDataBuffer.append(msg);

    // 检查缓冲区是否包含完整的一行数据
    while (receivedDataBuffer.contains('\n')) {
        int newlineIndex = receivedDataBuffer.indexOf('\n');
        QByteArray lineData = receivedDataBuffer.left(newlineIndex + 1); // 提取一行数据
        receivedDataBuffer.remove(0, newlineIndex + 1); // 从缓冲区中移除已处理的数据

        // 执行一些针对串口数据的操作，例如解析数据、转换格式等
        QString strmsg = QString::fromUtf8(lineData);
        if(strmsg.startsWith("$GNRMC"))
        {
            QStringList line = strmsg.split(',');
            double x =(line[3].toDouble())/100;
            double y =(line[2].toDouble())/100;
            int id = line[1].toInt();

            // 将数据发送到 JavaScript 函数
            QString jsCode = QString("SetPoint(%1, %2, %3)").arg(QString::number(x), QString::number(y), QString::number(id));
            ui->MapWidget->page()->runJavaScript(jsCode);

            // 更新日志
            ui->comLog->insertPlainText(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") + " [receive] " + strmsg + "\n");
        }
    }
}


//void MainWindow::recvMsg()
//{
//    // 读取串口缓冲区中所有的数据
//    QByteArray msg = this->serialPort->readAll();

//    // 执行一些针对串口数据的操作，例如解析数据、转换格式等
//    QString strmsg = QString::fromUtf8(msg);
//    if(strmsg.startsWith("$GNRMC"))
//    {
//        QStringList line = strmsg.split(',');
////        double w =(line[3].toDouble())/100;
////        double j =(line[2].toDouble())/100;

//        double x =(line[3].toDouble())/100;
//        double y =(line[2].toDouble())/100;
//        int id = line[1].toInt();
//        // 将接收到的数据以普通字符串形式添加到UI界面的接收信息显示框中
////        QString jsCode = QString("SetPoint(%1, %2)").arg(QString::number(x), QString::number(y));
////        ui->MapWidget->page()->runJavaScript(QString("SetPoint(%1,%2)").arg(x).arg(y));

//        QString jsCode = QString("SetPoint(%1, %2, %3)").arg(QString::number(x), QString::number(y), QString::number(id));
//        ui->MapWidget->page()->runJavaScript(jsCode);

//        ui->comLog->insertPlainText(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") + " [receive] " + strmsg + "\n");
//        //ui->comLog->insertPlainText(strmsg);
//        // 调用处理经纬度的函数
////        handleCoordinates1(j, w);
//    }
////    else
////    {
////        // 将接收到的数据以普通字符串形式添加到UI界面的接收信息显示框中
////        //ui->comLog->insertPlainText(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") + " [receive] " + strmsg + "\n");
////    }
//    // 将接收到的数据以普通字符串形式添加到UI界面的接收信息显示框中
//    //ui->comLog->insertPlainText(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") + " [receive] " + msg.data() + "\n");
//}

