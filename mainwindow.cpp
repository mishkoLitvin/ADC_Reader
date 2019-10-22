
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

    valueLabel = new QLabel(this);
    ui->consoleLayout->addWidget(valueLabel);
    valueLabel->setText("000000");
    valueLabel->setFont(QFont("Times", 25, 1, true));
    valueLabel->setAlignment(Qt::AlignHCenter);

    console = new Console();
    ui->consoleLayout->addWidget(console);
//! [1]
    serial = new QSerialPort(this);
//! [1]
    settingsDialog = new SettingsDialog();

    this->setWindowTitle("ADC Reader");

    qApp->setStyleSheet("*{color: black; "
                        "background-color: #DDD; "
                        "selection-color: #FFF;"
                        "selection-background-color: #666};"
                        "QComboBox{background-color: #DDD; spacing: 5px;};"
                        "QCheckBox::indicator{ min-width: 30px; min-height: 30px;}; "
                        "QComboBox::drop-down { subcontrol-origin: padding; "
                        "subcontrol-position: top right; "
                        "width: 15px; "
                        "border-left-width: 1px; "
                        "border-left-color: darkgray; "
                        "border-left-style: solid; "
                        "border-top-right-radius: 3px; "
                        "border-bottom-right-radius: 3px;};"
                        "QPushButton{"
                        "background-color: red;"
                        "border-style: outset;"
                        "border-width: 2px;"
                        "border-color: beige;"
                        "}"
                        );
    console->setStyleSheet("*{color: #1F1;"
                           "background-color: #111; "
                           "selection-color: #111; "
                           "selection-background-color: #1F1};");
    settingsDialog->setStyleSheet(qApp->styleSheet());

    alertLabel = new QLabel(this);
    ui->consoleLayout->addWidget(alertLabel);
    alertLabel->setText("BEWARE! \nVOLTAGE IS TOO HIGH!!");
    alertLabel->setFont(QFont("Times", 25, 1, true));

    alertLabel->setAlignment(Qt::AlignHCenter);
    alertLabel->setVisible(false);

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

void MainWindow::closeSerialPort()
{
    if ((serial->isOpen()))
        serial->close();
    ui->actionConnect->setEnabled(true);
    ui->actionDisconnect->setEnabled(false);
    ui->actionConfigure->setEnabled(true);

    showStatusMessage(tr("Disconnected"));

    if(m_logFile.isOpen()){
        m_logFile.flush();
        m_logFile.close();
    }
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
    serialData.append(serial->readAll());
    if(serialData.length()>2)
    {
        QString text = QString::number(m_mainDataCounter++)+"\t";

        int data =
                static_cast<int>(((static_cast<uint8_t>(serialData[0]))<<16) & 0x00FF0000)
                | static_cast<int>(((static_cast<uint8_t>(serialData[1]))<<8) & 0x0000FF00)
                | static_cast<int>(((static_cast<uint8_t>(serialData[2]))<<0 ) & 0x000000FF);
        if(data>0x400000)
            data = data-0x7FFFFF;

        double k = static_cast<double>(0x3FFFFF)*2./2.5;
        double voltage = static_cast<double>(data)/k*1000.;

        alertLabel->setVisible(fabs(voltage)>1200);
        if(m_mainDataCounter%10 == 0)
            valueLabel->setNum(voltage);

        text += QString::number(data)+"\t"+QString::number(voltage, 'f', 9)+"\t";
        if(m_logFile.isOpen()){
            m_out<<text<<"\n";
            m_out.flush();
        }

        if(recordData && m_dataFile.isOpen())
        {
           m_dataOut<<m_mainDataCounter/10.0<<"\t"<<data<<"\n";
           m_dataOut.flush();
        }

        console->putData(text+"\n");
        graphicItem->appendData(m_mainDataCounter/10.0, data, 0);
        ui->progressBar->setValue(m_mainDataCounter%100);
        serialData.clear();
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
    if(m_dataFile.isOpen())
        m_dataFile.close();

    if(QFile::exists(c_dataFileName))
        QFile::remove(c_dataFileName);

    m_dataFile.setFileName(c_dataFileName);
    qDebug()<<"m_dataFile.open"<<m_dataFile.open(QIODevice::ReadWrite);
    m_dataOut.setDevice(&m_dataFile);
    resetLog();
    m_mainDataCounter = 0;
}

void MainWindow::setRecord(bool record)
{
    recordData = record;

    if(record)
        this->clearData();
    else {
        if(m_dataFile.isOpen())
            m_dataFile.close();
        if(QFile::exists(c_dataFileName))
            QFile::remove(c_dataFileName);
    }

}

void MainWindow::saveToFile()
{
    if(!recordData || !m_dataFile.isOpen())
    {
        QMessageBox msg(this);
        msg.setText("You have to turn on <Record> to have something to save.\n"
                    "Turn it on now?");
        msg.addButton(QMessageBox::Yes);
        msg.addButton(QMessageBox::No);

        int res = msg.exec();

        if(res == QMessageBox::Yes){
            recordData = true;
            ui->actionRecord->setChecked(true);
            return;
        }
        return;
    }



    QString fName = QFileDialog::getSaveFileName(this, "Save data", "/home/user/");
    if(m_dataFile.isOpen()){
        m_dataFile.flush();
        m_dataFile.close();
    }

    QFile::copy(c_dataFileName, fName);

    m_dataFile.open(QIODevice::ReadWrite);
    m_dataFile.seek(m_dataFile.size());
    m_dataOut.setDevice(&m_dataFile);
}

void MainWindow::powerOff()
{
    QMessageBox msg;
    msg.setText("Turn off PC?");
    msg.addButton(QMessageBox::Yes);
    msg.addButton(QMessageBox::No);
    QAbstractButton* gui = msg.addButton("Open GUI", QMessageBox::NoRole);

    int res = msg.exec();

    if(msg.clickedButton() == gui) {
        system("startx");
        this->close();
    }
    else {
        if(res == QMessageBox::Yes){
            if(m_logFile.isOpen()){
                m_logFile.flush();
                m_logFile.close();
            }

            if(recordData && m_dataFile.isOpen()){
                QMessageBox msg;
                msg.setText("<Record> was enabled!\n"
                            "Do You want to save data to file?");
                msg.addButton(QMessageBox::Yes);
                msg.addButton(QMessageBox::No);
                int res2 = msg.exec();

                if(res2 == QMessageBox::Yes){
                    this->saveToFile();
                    return;
                }
            }

            if(m_dataFile.isOpen())
                m_dataFile.close();
            if(QFile::exists(c_dataFileName))
                QFile::remove(c_dataFileName);

            closeSerialPort();
            system("poweroff");
        }
    }

}


void MainWindow::exitFromApp()
{
    QMessageBox msgBox;
    msgBox.setText("Exit from application?");
    msgBox.addButton(QMessageBox::Yes);
    msgBox.addButton(QMessageBox::No);

    QAbstractButton* off = msgBox.addButton("Power Off", QMessageBox::YesRole);
    QAbstractButton* gui = msgBox.addButton("Open GUI", QMessageBox::NoRole);

    int res = msgBox.exec();
    qDebug()<<":::"<<res;
    if(msgBox.clickedButton() == off)
        this->powerOff();
    else
        if(msgBox.clickedButton() == gui) {
            system("startx");
            this->close();
        }
        else {
            if(res == QMessageBox::Yes){
                if(m_logFile.isOpen()){
                    m_logFile.flush();
                    m_logFile.close();
                }

                if(recordData && m_dataFile.isOpen()){
                    QMessageBox msg;
                    msg.setText("<Record> was enabled!\n"
                                "Do You want to save data to file?");
                    msg.addButton(QMessageBox::Save);
                    msg.addButton(QMessageBox::No);
                    if(res == QMessageBox::Save){
                        this->saveToFile();
                    }
                }

                if(m_dataFile.isOpen())
                    m_dataFile.close();
                if(QFile::exists(c_dataFileName))
                    QFile::remove(c_dataFileName);

                closeSerialPort();
                close();
            }

        }

}


void MainWindow::initActionsConnections()
{
    connect(ui->actionConnect, &QAction::triggered, this, &MainWindow::openSerialPort);
    connect(ui->actionDisconnect, &QAction::triggered, this, &MainWindow::closeSerialPort);
    connect(ui->actionQuit, &QAction::triggered, this, &MainWindow::exitFromApp);
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

void MainWindow::resetLog()
{
    if(m_logFile.isOpen()){
        m_logFile.flush();
        m_logFile.close();
    }

    QDir dir;
    dir.setPath("/home/user/Log/");
    qDebug()<<"log dir.exists"<<dir.exists();
    if(!dir.exists())
        qDebug()<<dir.mkdir("/home/user/Log/");

    m_logFile.setFileName("/home/user/Log/"+QDateTime::currentDateTime().toString("yyMMdd_HHmmss"));
    m_logFile.open(QIODevice::ReadWrite);
    m_out.setDevice(&m_logFile);
}
