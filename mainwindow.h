#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QThread>
#include <QTimer>
#include <QButtonGroup>
#include "worker.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void selectOutputDir();
    void startProcessing();
    void runProcessing();
    void onWorkerProgress(qint64 processed, qint64 total);
    void onWorkerFinished();
    void onTimerTimeout();

private:
    Ui::MainWindow *ui;
    Worker *worker;
    QThread *workerThread;
    QTimer *pollTimer;
    QButtonGroup *modeGroup;
    QButtonGroup *conflictGroup;
};
#endif // MAINWINDOW_H
