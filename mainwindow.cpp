
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "console.h"
#include "settingsdialog.h"

#include <QMessageBox>
#include <QLabel>
#include <QtSerialPort/QSerialPort>
#include <QtSerialPort/QSerialPortInfo>

#include <QDir>


//! [0]
MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
//! [0]
    ui->setupUi(this);

    console = new Console();
    ui->consoleLayout->addWidget(console);
//! [1]
    serial = new QSerialPort(this);
//! [1]
    settingsDialog = new SettingsDialog();

    this->setWindowTitle("ADC Reader");

    ui->actionConnect->setEnabled(true);
    ui->actionDisconnect->setEnabled(false);
    ui->actionQuit->setEnabled(true);
    ui->actionConfigure->setEnabled(true);

    status = new QLabel;
    ui->statusBar->addWidget(status);

    initActionsConnections();

    connect(serial, static_cast<void (QSerialPort::*)(QSerialPort::SerialPortError)>(&QSerialPort::error),
            this, &MainWindow::handleError);

    connect(serial, &QSerialPort::readyRead, this, &MainWindow::readData);
    connect(console, &Console::getData, this, &MainWindow::writeData);

    graphicItem = new GraphWidg(this, 1, "Intensivity");
    ui->graphicLayout->addWidget(graphicItem->worker);

    this->clearData();

    if(settingsDialog->settings().autoConnectEnabled){
        this->openSerialPort();
    }
}
//! [3]

MainWindow::~MainWindow()
{
    delete settingsDialog;
    delete ui;
}

void MainWindow::openSerialPortName(QString portName)
{
    if(serial->isOpen())
        return;

    SettingsDialog::Settings p = settingsDialog->settings();
    serial->setPortName(portName);
    serial->setBaudRate(p.baudRate);
    serial->setDataBits(p.dataBits);
    serial->setParity(p.parity);
    serial->setStopBits(p.stopBits);
    serial->setFlowControl(p.flowControl);
    serial->setReadBufferSize(2048);

    if (serial->open(QIODevice::ReadWrite)) {
        console->setEnabled(true);
        console->setLocalEchoEnabled(p.localEchoEnabled);
        ui->actionConnect->setEnabled(false);
        ui->actionDisconnect->setEnabled(true);
        ui->actionConfigure->setEnabled(false);
        showStatusMessage(tr("Connected to %1 : %2, %3, %4, %5, %6")
                          .arg(p.name).arg(p.stringBaudRate).arg(p.stringDataBits)
                          .arg(p.stringParity).arg(p.stringStopBits).arg(p.stringFlowControl));
    } else {
        showStatusMessage(serial->errorString());
    }
}

//! [4]
void MainWindow::openSerialPort()
{
    SettingsDialog::Settings p = settingsDialog->settings();
    serial->setPortName(p.name);
    serial->setBaudRate(p.baudRate);
    serial->setDataBits(p.dataBits);
    serial->setParity(p.parity);
    serial->setStopBits(p.stopBits);
    serial->setFlowControl(p.flowControl);
    serial->setReadBufferSize(2048);

    if (serial->open(QIODevice::ReadWrite)) {
        console->setEnabled(true);
        console->setLocalEchoEnabled(p.localEchoEnabled);
        ui->actionConnect->setEnabled(false);
        ui->actionDisconnect->setEnabled(true);
        ui->actionConfigure->setEnabled(false);
        showStatusMessage(tr("Connected to %1 : %2, %3, %4, %5, %6")
                          .arg(p.name).arg(p.stringBaudRate).arg(p.stringDataBits)
                          .arg(p.stringParity).arg(p.stringStopBits).arg(p.stringFlowControl));
    } else {
        showStatusMessage(serial->errorString());
    }
}
//! [4]

//! [5]
void MainWindow::closeSerialPort()
{
    if ((serial->isOpen()))
        serial->close();
    ui->actionConnect->setEnabled(true);
    ui->actionDisconnect->setEnabled(false);
    ui->actionConfigure->setEnabled(true);

    showStatusMessage(tr("Disconnected"));
}
//! [5]

void MainWindow::about()
{
    QMessageBox::about(this, tr("About Simple Terminal"),
                       tr("The <b>Simple Terminal</b> example demonstrates how to "
                          "use the Qt Serial Port module in modern GUI applications "
                          "using Qt, with a menu bar, toolbars, and a status bar."));
}

//! [6]
void MainWindow::writeData(const QByteArray &data)
{
    serial->write(data);
}
//! [6]

//! [7]
void MainWindow::readData()
{
    static int ggg = 0;
    serialData.append(serial->readAll());
    QByteArray tempData;
    if(serialData.length()>4)
    {
        QString text = QString::number(ggg++)+"\t";

        int data =
                static_cast<int>(((static_cast<uint8_t>(serialData[0]))<<16) & 0x00FF0000)
                | static_cast<int>(((static_cast<uint8_t>(serialData[1]))<<8) & 0x0000FF00)
                | static_cast<int>(((static_cast<uint8_t>(serialData[2]))<<0 ) & 0x000000FF);
        if(data>0x400000)
            data = data-0x7FFFFF;

        double k = static_cast<double>(0x3FFFFF)*2/2.5;
        double voltage = static_cast<double>(data)/k;
        text += QString::number(data)+"\t"+QString::number(voltage, 'f', 9)+"\t";
        if(recordData)
        {
            allData.append(QPointF(ggg/10.0, data));
        }
        console->putData(text+"\n");
        graphicItem->appendData(ggg/10.0, data, 0);
        ui->progressBar->setValue(ggg%100);
    }
}

void MainWindow::handleError(QSerialPort::SerialPortError error)
{
    qDebug()<<(error == QSerialPort::ResourceError)<<serial->isOpen();
    this->closeSerialPort();

    showStatusMessage(serial->errorString());
}

void MainWindow::clearData()
{
    console->clear();
    graphicItem->clear();
    allData.clear();
}

void MainWindow::setRecord(bool record)
{
    recordData = record;
    if(record)
        this->clearData();
}

void MainWindow::saveToFile()
{
    QString fName = QFileDialog::getSaveFileName(this, "Save data", QDir::homePath());
    QFile file(fName);
    file.open(QIODevice::WriteOnly);
    QTextStream out(&file);
    for (int i(0); i<allData.length(); i++) {
        out<<allData.at(i).x()<<"\t"<<allData.at(i).y()<<"\n";
    }
    file.flush();
    file.close();
}

void MainWindow::powerOff()
{
    QMessageBox msg;
    msg.setText("Turn off PC?");
    msg.addButton(QMessageBox::Yes);
    msg.addButton(QMessageBox::No);

    int res = msg.exec();
    if(res == QMessageBox::Yes){
        closeSerialPort();
        system("poweroff");
    }

}


void MainWindow::initActionsConnections()
{
    connect(ui->actionConnect, &QAction::triggered, this, &MainWindow::openSerialPort);
    connect(ui->actionDisconnect, &QAction::triggered, this, &MainWindow::closeSerialPort);
    connect(ui->actionQuit, &QAction::triggered, this, &MainWindow::close);
    connect(ui->actionTurn_Off, &QAction::triggered, this, &MainWindow::powerOff);

    connect(ui->actionConfigure, &QAction::triggered, settingsDialog, &MainWindow::show);
    connect(ui->actionClear, &QAction::triggered, this, &MainWindow::clearData);
    connect(ui->actionAbout, &QAction::triggered, this, &MainWindow::about);
    connect(ui->actionAboutQt, &QAction::triggered, qApp, &QApplication::aboutQt);

    connect(ui->actionSave, &QAction::triggered, this, &MainWindow::saveToFile);
    connect(ui->actionRecord, &QAction::triggered, this, &MainWindow::setRecord);

}

void MainWindow::showStatusMessage(const QString &message)
{
    status->setText(message);
}
