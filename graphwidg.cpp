#include "graphwidg.h"
#include "ui_graphwidg.h"

#include <algorithm>
#include <QDebug>
#include <QFile>
#include <QFileDialog>

GraphWidgWorker::GraphWidgWorker(QWidget *parent, int graphCount, QString name) :
    QWidget(parent),
    ui(new Ui::GraphWidgWorker)
{
    ui->setupUi(this);

    plot = new QCustomPlot(this);
    ui->layoutPlotter->addWidget(plot);

    textEdit = new QTextEdit(this);
    ui->layoutData->addWidget(textEdit);
    ui->widget->setVisible(false);

    saveDataButton = new QPushButton(this);
    saveDataButton->setText("Save data");
    ui->layoutData->addWidget(saveDataButton);

    closeButton = new QPushButton(this);
    closeButton->setText("Close");
    ui->layoutData->addWidget(closeButton);

    connect(closeButton, &QPushButton::clicked, this, &GraphWidgWorker::closeData);

    this->setWindowTitle(name);

    allGraphCount = graphCount;

    for(size_t i = 0; i<graphCount; i++)
    {
        plot->addGraph();

        plot->graph(i)->setPen(QPen(QColor(255/graphCount*i,0,255-255/graphCount*i)));
        plot->graph(i)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssCircle));

    }
    plot->setInteractions(QCP::iRangeDrag|QCP::iRangeZoom); // this will also allow rescaling the color scale by dragging/zooming
    internalCounter = 0;

    x = new QVector<double>[graphCount];
    y = new QVector<double>[graphCount];
    xMin.reserve(graphCount);
    yMin.reserve(graphCount);
    xMax.reserve(graphCount);
    yMax.reserve(graphCount);

    connect(ui->spinBoxXMax,SIGNAL(valueChanged(double)),this,SLOT(setGraphArea(double)));
    connect(ui->spinBoxXMin,SIGNAL(valueChanged(double)),this,SLOT(setGraphArea(double)));
    connect(ui->spinBoxYMax,SIGNAL(valueChanged(double)),this,SLOT(setGraphArea(double)));
    connect(ui->spinBoxYMin,SIGNAL(valueChanged(double)),this,SLOT(setGraphArea(double)));

    connect(ui->pushButtonSettings,SIGNAL(clicked(bool)), this,SLOT(showSetting(bool)));
    connect(ui->pushButtonStop,SIGNAL(clicked(bool)), this, SLOT(stopBtnPressed()));
    connect(ui->pushButtonData, SIGNAL(clicked(bool)), this, SLOT(getData()));

    ui->widgetSettings->setHidden(!ui->pushButtonSettings->isChecked());

    QTimer *tim = new QTimer(this);
    connect(tim, &QTimer::timeout, this, &GraphWidgWorker::replotTimerTick);
    tim->start(100);

}


GraphWidgWorker::~GraphWidgWorker()
{
    delete ui;
}

void GraphWidgWorker::closeEvent(QCloseEvent *e)
{
    this->setWindowState(Qt::WindowMinimized);
    e->ignore();
}

void GraphWidgWorker::setGraphArea(double val)
{
    if(!ui->checkBoxXAuto->isChecked())
        plot->xAxis->setRange(ui->spinBoxXMin->value(),
                              ui->spinBoxXMax->value());
    if(!ui->checkBoxYAuto->isChecked())
        plot->yAxis->setRange(ui->spinBoxYMin->value(),
                              ui->spinBoxYMax->value());
    plot->replot();
}

void GraphWidgWorker::showSetting(bool sh)
{
    ui->widgetSettings->setHidden(!ui->pushButtonSettings->isChecked());
}

void GraphWidgWorker::stopBtnPressed()
{
    ui->pushButtonData->setEnabled(ui->pushButtonStop->isChecked());

}

void GraphWidgWorker::getData()
{
    size_t i, j;
    textEdit->clear();
    QString tempStr;
    for(i = 0; i<x->size(); i++)
    {
        tempStr = "";
        for(j = 0; j<allGraphCount; j++)
        {
            tempStr += QString::number(x[j][i])+"\t"+QString::number(y[j][i]);
        }
        textEdit->append(tempStr);
    }

    ui->widget->setVisible(true);
}

void GraphWidgWorker::closeData()
{
    ui->widget->setVisible(false);
}

void GraphWidgWorker::saveData()
{
    QString fName = QFileDialog::getSaveFileName(this, "Save data", QDir::homePath());
    QFile file(fName);
    file.open(QIODevice::WriteOnly);
    file.write(textEdit->toPlainText().toLocal8Bit());
    file.flush();
    file.close();
    ui->widget->setVisible(false);

}

void GraphWidgWorker::replotTimerTick()
{
    double min, max;

    if(ui->checkBoxXAuto->isChecked())
    {
        xMin[0] = *std::min_element(x[0].begin(), x[0].end());
        xMax[0] = *std::max_element(x[0].begin(), x[0].end());
        min = *std::min_element(xMin.begin(), xMin.end());
        max = *std::max_element(xMax.begin(), xMax.end());
        plot->xAxis->setRange(min,max);
        if(ui->pushButtonSettings->isChecked()) {
            ui->spinBoxXMin->setValue(min);
            ui->spinBoxXMax->setValue(max);
        }
    }
    if(ui->checkBoxYAuto->isChecked())
    {
        yMin[0] = *std::min_element(y[0].begin(), y[0].end());
        yMax[0] = *std::max_element(y[0].begin(), y[0].end());
        min = *std::min_element(yMin.begin(), yMin.end());
        max = *std::max_element(yMax.begin(), yMax.end());
        plot->yAxis->setRange(min,max);
        if(ui->pushButtonSettings->isChecked()) {
            ui->spinBoxYMin->setValue(min);
            ui->spinBoxYMax->setValue(max);
        }
    }

    plot->replot();
}

void GraphWidgWorker::setData(QVector<double> xData, QVector<double> yData, int graphIndex)
{
    if(graphIndex<0 || graphIndex>allGraphCount)
    {
        qWarning()<<"Graph index is not an goot one:"<<graphIndex;
        return;
    }
    if(!ui->pushButtonStop->isChecked())
    {
        x[graphIndex] = xData;
        y[graphIndex] = yData;

        double min, max;
        plot->graph(graphIndex)->setData(x[graphIndex], y[graphIndex]);
        if(ui->checkBoxXAuto->isChecked())
        {

            xMin[graphIndex] = *std::min_element(x[graphIndex].begin(), x[graphIndex].end());
            xMax[graphIndex] = *std::max_element(x[graphIndex].begin(), x[graphIndex].end());
            min = *std::min_element(xMin.begin(), xMin.end());
            max = *std::max_element(xMax.begin(), xMax.end());
            plot->xAxis->setRange(min,max);
            if(ui->pushButtonSettings->isChecked()) {
                ui->spinBoxXMin->setValue(min);
                ui->spinBoxXMax->setValue(max);
            }
        }
        if(ui->checkBoxYAuto->isChecked())
        {
            yMin[graphIndex] = *std::min_element(y[graphIndex].begin(), y[graphIndex].end());
            yMax[graphIndex] = *std::max_element(y[graphIndex].begin(), y[graphIndex].end());
            min = *std::min_element(yMin.begin(), yMin.end());
            max = *std::max_element(yMax.begin(), yMax.end());
            plot->yAxis->setRange(min,max);
            if(ui->pushButtonSettings->isChecked()) {
                ui->spinBoxYMin->setValue(min);
                ui->spinBoxYMax->setValue(max);
            }
        }
        plot->replot();
    }

}

void GraphWidgWorker::appendData(double xN, double yN, int graphIndex)
{
    if(graphIndex<0 || graphIndex>allGraphCount)
    {
        qWarning()<<"Graph index is not an goot one:"<<graphIndex;
        return;
    }
    if(!ui->pushButtonStop->isChecked())
    {
        x[graphIndex].push_back(xN);
        y[graphIndex].push_back(yN);
        if(x[graphIndex].size()>300)
        {
            x->erase(x->begin());
            y->erase(y->begin());
        }
        plot->graph(graphIndex)->setData(x[graphIndex], y[graphIndex]);
    }
}

void GraphWidgWorker::appendYPoint(double yN, int graphIndex)
{
    this->appendData(internalCounter++, yN, graphIndex);
}

void GraphWidgWorker::clear()
{
    for (size_t i(0); i<allGraphCount; i++) {
        x[i].clear();
        y[i].clear();
        plot->graph(i)->setData(x[i], y[i]);
    }
}

