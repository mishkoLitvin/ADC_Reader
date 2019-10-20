
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
    settings = new SettingsDialog();


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

    outFile.setFileName(QDir::homePath()+"/Temp/RAW/"+QTime::currentTime().toString("hh_mm_ss")+".txt");
    outFile.open(QIODevice::ReadWrite);
    outStream.setDevice(&outFile);
}
//! [3]

MainWindow::~MainWindow()
{
    outFile.close();
    delete settings;
    delete ui;
}

void MainWindow::openSerialPortName(QString portName)
{
    if(serial->isOpen())
        return;

    SettingsDialog::Settings p = settings->settings();
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
    SettingsDialog::Settings p = settings->settings();
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
        outStream<<serialData;

        QString text = QString::number(ggg++)+"\t";

        int data =
                static_cast<int>((((uint8_t)serialData[0])<<16) & 0x00FF0000)
                | static_cast<int>((((uint8_t)serialData[1])<<8) & 0x0000FF00)
                | static_cast<int>((((uint8_t)serialData[2])<<0 ) & 0x000000FF);
        if(data>0x400000)
            data = data-0x7FFFFF;

        double k = static_cast<double>(0x3FFFFF)*2/2.5;
        double voltage = static_cast<double>(data)/k;
        text += QString::number(data)+"\t"+QString::number(voltage, 'f', 9)+"\t";
        console->putData(text+"\n");
        graphicItem->appendData(ggg/10.0, voltage, 0);
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
}


void MainWindow::initActionsConnections()
{
    connect(ui->actionConnect, &QAction::triggered, this, &MainWindow::openSerialPort);
    connect(ui->actionDisconnect, &QAction::triggered, this, &MainWindow::closeSerialPort);
    connect(ui->actionQuit, &QAction::triggered, this, &MainWindow::close);
    connect(ui->actionConfigure, &QAction::triggered, settings, &MainWindow::show);
    connect(ui->actionClear, &QAction::triggered, console, &Console::clear);
    connect(ui->actionClear, SIGNAL(triggered(bool)), this, SLOT(clearData()));
    connect(ui->actionAbout, &QAction::triggered, this, &MainWindow::about);
    connect(ui->actionAboutQt, &QAction::triggered, qApp, &QApplication::aboutQt);
}

void MainWindow::showStatusMessage(const QString &message)
{
    status->setText(message);
}
