#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // Set up vidRenderer appearance
    ui->vidRenderer->setStyleSheet("background-color: black;");

    // Create the NetworkWorker and connect signals
    networkWorker = new NetworkWorker();

    connect(networkWorker, &NetworkWorker::newFrame, this, &MainWindow::updateFrame);
    connect(networkWorker, &NetworkWorker::connected, this, &MainWindow::onConnected);
    connect(networkWorker, &NetworkWorker::disconnected, this, &MainWindow::onDisconnected);
    connect(networkWorker, &NetworkWorker::errorOccurred, this, &MainWindow::onError);

    // Connect the Start button (assuming you have one in the UI)
    connect(ui->startButton, &QPushButton::clicked, this, &MainWindow::onStartButtonClicked);

    // Connect to the server via your SSH tunnel
    networkWorker->connectToServer("127.0.0.1", 7000);  // Change port if needed

    // ✅ Start the frame timer
    frameTimer.start();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::updateFrame(const QImage &img)
{
    const qint64 minIntervalMs = 1000 / 3;  // ~33 ms per frame for 30 FPS

    // ✅ Only render if enough time has passed
    if (frameTimer.elapsed() < minIntervalMs) {
        // qDebug() << "Skipping frame to maintain 30 FPS.";  // Optional debug log
        return;
    }

    // Render the frame
    ui->vidRenderer->setPixmap(QPixmap::fromImage(img).scaled(
        ui->vidRenderer->size(),
        Qt::KeepAspectRatio,
        Qt::SmoothTransformation
        ));

    // Restart timer for next frame
    frameTimer.restart();
}

void MainWindow::onStartButtonClicked()
{
    // Example: Send a "start simulation" command
    QString command = "run ./simulator";  // Change to your actual simulator command
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
