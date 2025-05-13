#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QFileDialog>


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_browseButton_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(this, "Select a file");
    if (!fileName.isEmpty()) {
        ui->filePathEdit->setText(fileName);  // If you have a QLineEdit to show the path
        qDebug() << "Selected file:" << fileName;
    }
}

