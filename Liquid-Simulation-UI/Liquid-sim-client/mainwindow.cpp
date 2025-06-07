#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QThread>
#include <QDebug>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->vidRenderer->setStyleSheet("background-color: black;");

    // Set up worker thread
    QThread *netThread = new QThread(this);
    networkWorker = new NetworkWorker();
    networkWorker->moveToThread(netThread);

    // Safely call connectToServer from inside the worker thread
    connect(netThread, &QThread::started, [this]() {
        networkWorker->connectToServer("127.0.0.1", 7000);
    });

    // Connect worker signals
    connect(networkWorker, &NetworkWorker::rawFrameDataReceived, this, &MainWindow::handleRawFrameData);
    connect(networkWorker, &NetworkWorker::connected, this, &MainWindow::onConnected);
    connect(networkWorker, &NetworkWorker::disconnected, this, &MainWindow::onDisconnected);
    connect(networkWorker, &NetworkWorker::errorOccurred, this, &MainWindow::onError);

    // Thread cleanup
    connect(netThread, &QThread::finished, networkWorker, &QObject::deleteLater);

    // Start thread
    netThread->start();

    // Frame playback
    connect(&playbackTimer, &QTimer::timeout, this, &MainWindow::playBufferedFrame);

    // Start button
    connect(ui->startButton, &QPushButton::clicked, this, &MainWindow::onStartButtonClicked);
}

MainWindow::~MainWindow()
{
    QThread *netThread = networkWorker->thread();
    netThread->quit();
    netThread->wait();
    delete ui;
}

void MainWindow::handleRawFrameData(QByteArray rawData, int frameCount, int width, int height)
{
    rawFrameData = std::move(rawData);
    totalFrames = frameCount;
    currentFrameIndex = 0;
    frameWidth = width;
    frameHeight = height;

    qDebug() << "Received" << frameCount << "frames of size" << width << "x" << height;

    if (!playbackStarted && totalFrames > 0) {
        playbackStarted = true;
        playbackTimer.start(55);  // ~30 FPS
    }
}

void MainWindow::playBufferedFrame()
{
    if (currentFrameIndex >= totalFrames) {
        playbackTimer.stop();
        playbackStarted = false;
        qDebug() << "All frames displayed.";
        return;
    }

    const int frameSize = frameWidth * frameHeight * 4;
    const int offset = currentFrameIndex * frameSize;

    if (offset + frameSize > rawFrameData.size()) {
        qWarning() << "Frame offset out of range!";
        playbackTimer.stop();
        playbackStarted = false;
        return;
    }

    QImage img(reinterpret_cast<const uchar*>(rawFrameData.constData() + offset),
               frameWidth, frameHeight, QImage::Format_RGBA8888);

    if (!img.isNull()) {
        ui->vidRenderer->setPixmap(QPixmap::fromImage(img).scaled(
            ui->vidRenderer->size(),
            Qt::KeepAspectRatio,
            Qt::SmoothTransformation));
    } else {
        qWarning() << "Invalid image at frame" << currentFrameIndex;
    }

    ++currentFrameIndex;
}

void MainWindow::onStartButtonClicked()
{
    // Example: Send a "start simulation" command
    // QString command = "run ./simulator";  // Change to your actual simulator command
    QString command = "run salloc " + ui->imageselect->currentText();
    networkWorker->sendCommand(command);
}

void MainWindow::onConnected()
{
    ui->statusbar->showMessage("Connected to server");
}

void MainWindow::onDisconnected()
{
    ui->statusbar->showMessage("Disconnected from server");
}

void MainWindow::onError(QString error)
{
    ui->statusbar->showMessage("Error: " + error);
}
