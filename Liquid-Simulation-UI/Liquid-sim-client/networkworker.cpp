#include "networkworker.h"
#include <QDataStream>
#include <QDebug>
#include <QDateTime>


NetworkWorker::NetworkWorker(QObject *parent)
    : QObject(parent)
{
    socket = new QTcpSocket(this);

    connect(socket, &QTcpSocket::readyRead, this, &NetworkWorker::onReadyRead);
    connect(socket, &QTcpSocket::connected, this, &NetworkWorker::onConnected);
    connect(socket, &QTcpSocket::disconnected, this, &NetworkWorker::onDisconnected);
    connect(socket, QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::errorOccurred),
            this, &NetworkWorker::onError);
}

NetworkWorker::~NetworkWorker()
{
    if (socket->isOpen()) {
        socket->close();
    }
}

void NetworkWorker::connectToServer(const QString &host, quint16 port)
{
    qDebug() << "Connecting to" << host << ":" << port;

    // 🚀 Set a bigger receive buffer (e.g., 10 MB)
    socket->setSocketOption(QAbstractSocket::ReceiveBufferSizeSocketOption, 1024 * 1024 * 10);  // 10 MB

    socket->connectToHost(host, port);
}

void NetworkWorker::sendCommand(const QString &command)
{
    if (!socket->isOpen()) {
        qDebug() << "Socket not connected!";
        return;
    }

    QByteArray data = command.toUtf8();
    qint32 size = data.size();

    QByteArray packet;
    QDataStream stream(&packet, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::LittleEndian);
    stream << size;
    packet.append(data);

    socket->write(packet);
    socket->flush();
}

void NetworkWorker::onConnected()
{
    qDebug() << "Connected to server!";
    emit connected();
}

void NetworkWorker::onDisconnected()
{
    qDebug() << "Disconnected from server!";
    emit disconnected();
}

void NetworkWorker::onError(QAbstractSocket::SocketError socketError)
{
    Q_UNUSED(socketError)
    qDebug() << "Socket error:" << socket->errorString();
    emit errorOccurred(socket->errorString());
}

void NetworkWorker::onReadyRead()
{
    while (socket->bytesAvailable() > 0) {
        buffer.append(socket->readAll());

        while (true) {
            // Step 1: Read the frame size (first 4 bytes)
            if (expectedFrameSize < 0) {
                if (buffer.size() >= 4) {
                    QDataStream stream(buffer.left(4));
                    stream.setByteOrder(QDataStream::LittleEndian);
                    stream >> expectedFrameSize;
                    buffer.remove(0, 4);
                } else {
                    // Not enough data yet
                    break;
                }
            }

            // Step 2: Read the full frame
            if (expectedFrameSize >= 0 && buffer.size() >= expectedFrameSize) {
                qDebug() << "Received frame at" << QDateTime::currentDateTime().toString("hh:mm:ss.zzz");
                QByteArray frameData = buffer.left(expectedFrameSize);
                buffer.remove(0, expectedFrameSize);
                expectedFrameSize = -1;

                QImage img = QImage::fromData(frameData);
                if (!img.isNull()) {
                    emit newFrame(img);
                } else {
                    qDebug() << "Failed to decode image frame.";
                }
            } else {
                // Not enough data yet for full frame
                break;
            }
        }
    }
}
