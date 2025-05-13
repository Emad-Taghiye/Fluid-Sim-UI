#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QElapsedTimer>
#include "networkworker.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void updateFrame(const QImage &img);
    void onStartButtonClicked();
    void onConnected();
    void onDisconnected();
    void onError(QString error);

private:
    Ui::MainWindow *ui;
    NetworkWorker *networkWorker;
    QElapsedTimer frameTimer;  // ✅ Added timer to throttle to 30 FPS
};

#endif // MAINWINDOW_H
