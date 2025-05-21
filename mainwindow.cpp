#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFileDialog>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , worker(nullptr)
    , workerThread(nullptr)
    , pollTimer(new QTimer(this))
{
    ui->setupUi(this);

    setFixedSize(size());

    modeGroup = new QButtonGroup(this);
    modeGroup->addButton(ui->radioSingle);
    modeGroup->addButton(ui->radioTimer);
    modeGroup->setExclusive(true);

    conflictGroup = new QButtonGroup(this);
    conflictGroup->addButton(ui->radioOverwrite);
    conflictGroup->addButton(ui->radioRename);
    conflictGroup->setExclusive(true);

    // default ui
    ui->radioSingle->setChecked(true);
    ui->radioRename->setChecked(true);
    ui->progressBar->setValue(0);

    connect(ui->btnBrowse, &QPushButton::clicked, this, &MainWindow::selectOutputDir);
    connect(ui->btnStart, &QPushButton::clicked, this, &MainWindow::startProcessing);
    connect(pollTimer, &QTimer::timeout, this, &MainWindow::onTimerTimeout);
}

MainWindow::~MainWindow()
{
    if(workerThread && workerThread->isRunning()){
        workerThread->quit();
        workerThread->wait();
    }
    delete ui;
}

void MainWindow::selectOutputDir()
{
    QString dir = QFileDialog::getExistingDirectory(this, tr("Select Output directory"));
    if(!dir.isEmpty()){
        ui->lineEditOutput->setText(dir);
    }
}

void MainWindow::startProcessing()
{
    if(workerThread && workerThread->isRunning()){
        ui->statusBar->showMessage(tr("Processing already in progress"));
        return;
    }

    ui->btnStart->setEnabled(false);
    if(ui->radioOverwrite->isChecked()){
        int interval = ui->spinInterval->value();
        pollTimer->start(interval*1000);
        ui->statusBar->showMessage(tr("Timer started: every %1 seconds").arg(interval));
    }
    else{
        pollTimer->stop();
        runProcessing();
    }
}

void MainWindow::runProcessing()
{
    if (workerThread && workerThread->isRunning()) {
        ui->statusBar->showMessage(tr("Still processing..."));
        return;
    }

    QStringList masks = ui->lineEditMask->text().split(',', Qt::SkipEmptyParts);
    for (QString &m : masks)
        m = m.trimmed();
    QString outDir = ui->lineEditOutput->text();
    bool removeInput = ui->checkBoxDeleteInputFiles->isChecked();
    bool overwrite = ui->radioOverwrite->isChecked();
    quint64 xorKey = ui->lineEditKey->text().toULongLong(nullptr, 16);

    worker = new Worker(masks, outDir, removeInput, overwrite, xorKey);
    workerThread = new QThread(this);
    worker->moveToThread(workerThread);
    connect(workerThread, &QThread::started, worker, &Worker::process);
    connect(worker, &Worker::progress, this, &MainWindow::onWorkerProgress);
    connect(worker, &Worker::finished, this, &MainWindow::onWorkerFinished);
    connect(worker, &Worker::finished, workerThread, &QThread::quit);
    connect(workerThread, &QThread::finished, worker, &QObject::deleteLater);
    connect(workerThread, &QThread::finished, workerThread, &QObject::deleteLater);

    ui->progressBar->setValue(0);
    workerThread->start();
    ui->statusBar->showMessage(tr("Processing started"));
}

void MainWindow::onTimerTimeout()
{
    runProcessing();
}

void MainWindow::onWorkerProgress(qint64 processed, qint64 total)
{
    ui->progressBar->setMaximum(total);
    ui->progressBar->setValue(processed);
}

void MainWindow::onWorkerFinished()
{
    ui->btnStart->setEnabled(true);
    ui->statusBar->showMessage(tr("Processing finished"));
    worker = nullptr;
    workerThread = nullptr;
}
