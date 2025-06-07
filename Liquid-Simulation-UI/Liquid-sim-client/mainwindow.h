#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QElapsedTimer>
#include "networkworker.h"
#include <QQueue>
#include <QTimer>
#include <QImage>

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
    void onStartButtonClicked();
    void onConnected();
    void onDisconnected();
    void onError(QString error);
    void handleRawFrameData(QByteArray rawData, int frameCount, int width, int height);
    void playBufferedFrame();


private:
    Ui::MainWindow *ui;
    NetworkWorker *networkWorker;
    QElapsedTimer frameTimer;  // ✅ Added timer to throttle to 30 FPS
    QQueue<QImage> frameBuffer;
    QTimer playbackTimer;
    bool playbackStarted = false;
    QByteArray rawFrameData;
    int totalFrameCount = 0;
    int currentFrameIndex = 0;
    int frameWidth = 0;
    int frameHeight = 0;
    int totalFrames = 0;

};

#endif // MAINWINDOW_H
