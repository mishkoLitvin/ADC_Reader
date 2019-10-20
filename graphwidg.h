#ifndef GRAPHWIDG_H
#define GRAPHWIDG_H

#include <QWidget>
#include "qcustomplot.h"
#include <QVector>
#include <QTextEdit>

#include <vector>


namespace Ui {
class GraphWidgWorker;
}

class GraphWidgWorker : public QWidget
{
    Q_OBJECT

public:
    explicit GraphWidgWorker(QWidget *parent = nullptr, int graphCount = 1, QString name = "NoName");

    ~GraphWidgWorker();

private:
    Ui::GraphWidgWorker *ui;

    QCustomPlot *plot;
    QVector<double> *x, *y, xT, yT;
    std::vector<double> xMin, yMin, xMax, yMax;
    size_t allGraphCount;
    uint64_t internalCounter;

    QTextEdit *textEdit;
    QPushButton *saveDataButton;
    QPushButton *closeButton;

    void closeEvent(QCloseEvent *e);

public slots:
    void setData(QVector<double> xData, QVector<double> yData, int graphIndex);
    void appendData(double xN, double yN, int graphIndex);
    void appendYPoint(double yN, int graphIndex);
    void clear();

private slots:
    void setGraphArea(double val);
    void showSetting(bool sh);
    void stopBtnPressed();
    void getData();
    void closeData();
    void saveData();
    void replotTimerTick();


};

class GraphWidg: public QObject
{
    Q_OBJECT

public:

    GraphWidgWorker *worker;
    QThread *workerThread;

    explicit GraphWidg(QWidget *parent = nullptr, int graphCount = 1, QString name = "NoName")
    {
        workerThread = new QThread(this);
        this->worker = new GraphWidgWorker(parent, graphCount, name);
        moveToThread(workerThread);
        workerThread->start();
    }

    void clear()
    {
        this->worker->clear();
    }

    void setData(QVector<double> xData, QVector<double> yData, int graphIndex)
    {
        this->worker->setData(xData, yData, graphIndex);
    }

    void appendData(double xN, double yN, int graphIndex)
    {
        QMetaObject::invokeMethod(worker, "appendData", Qt::QueuedConnection, Q_ARG(double, xN), Q_ARG(double, yN), Q_ARG(int, graphIndex));
//        this->worker->appendData(xN, yN, graphIndex);
    }

    void appendYPoint(double yN, int graphIndex)
    {
        QMetaObject::invokeMethod(worker, "appendYPoint", Qt::QueuedConnection, Q_ARG(double, yN), Q_ARG(int, graphIndex));
//        this->worker->appendYPoint(yN, graphIndex);
    }

};





#endif // GRAPHWIDG_H
