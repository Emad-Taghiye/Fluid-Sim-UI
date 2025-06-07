#include "networkworker.h"
#include <QDataStream>
#include <QDebug>
#include <QDateTime>
#include <QCoreApplication>



NetworkWorker::NetworkWorker(QObject *parent)
    : QObject(parent)
{
    socket = new QTcpSocket(this);

    connect(socket, &QTcpSocket::readyRead, this, &NetworkWorker::onReadyRead);
    connect(socket, &QTcpSocket::connected, this, &NetworkWorker::onConnected);
    connect(socket, &QTcpSocket::disconnected, this, &NetworkWorker::onDisconnected);
    connect(socket, QOverload<QAbstractSocket::SocketError>::of(&QTcpSocket::errorOccurred),
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

// void NetworkWorker::onReadyRead()
// {
//     while (socket->bytesAvailable() > 0) {
//         buffer.append(socket->readAll());

//         while (true) {
//             // Step 1: Read the frame size (first 4 bytes)
//             if (expectedFrameSize < 0) {
//                 if (buffer.size() >= 4) {
//                     QDataStream stream(buffer.left(4));
//                     stream.setByteOrder(QDataStream::LittleEndian);
//                     stream >> expectedFrameSize;
//                     buffer.remove(0, 4);
//                 } else {
//                     // Not enough data yet
//                     break;
//                 }
//             }

//             // Step 2: Read the full frame
//             if (expectedFrameSize >= 0 && buffer.size() >= expectedFrameSize) {
//                 qDebug() << "Received frame at" << QDateTime::currentDateTime().toString("hh:mm:ss.zzz");
//                 QByteArray frameData = buffer.left(expectedFrameSize);
//                 buffer.remove(0, expectedFrameSize);
//                 expectedFrameSize = -1;

//                 QImage img = QImage::fromData(frameData);
//                 if (!img.isNull()) {
//                     emit newFrame(img);
//                 } else {
//                     qDebug() << "Failed to decode image frame.";
//                 }
//             } else {
//                 // Not enough data yet for full frame
//                 break;
//             }
//         }
//     }
// }

// void NetworkWorker::onReadyRead()
// {
//     buffer.append(socket->readAll());

//     while (true) {
//         if (expectedFrameSize < 0) {
//             if (buffer.size() >= 8) {
//                 // Read width and height
//                 QDataStream stream(buffer.left(8));
//                 stream.setByteOrder(QDataStream::LittleEndian);
//                 stream >> frameWidth;
//                 stream >> frameHeight;
//                 expectedFrameSize = frameWidth * frameHeight * 4;
//                 buffer.remove(0, 8);
//             } else {
//                 break; // Wait for more data
//             }
//         }

//         if (expectedFrameSize >= 0 && buffer.size() >= expectedFrameSize) {
//             qDebug() << "Received frame at" << QDateTime::currentDateTime().toString("hh:mm:ss.zzz");
//             QByteArray frameData = buffer.left(expectedFrameSize);
//             buffer.remove(0, expectedFrameSize);
//             expectedFrameSize = -1;

//             // Construct QImage from raw RGBA
//             QImage img(reinterpret_cast<const uchar*>(frameData.data()),
//                        frameWidth, frameHeight, QImage::Format_RGBA8888);

//             emit newFrame(img.copy());  // emit deep copy to preserve frame
//         } else {
//             break; // Wait for more data
//         }
//     }
// }

void NetworkWorker::onReadyRead()
{
    buffer.append(socket->readAll());

    while (true) {
        // Step 1: Read metadata (frameCount, width, height = 3 × 4 bytes)
        if (expectedFrameSize < 0) {
            if (buffer.size() >= 12) {
                QDataStream stream(buffer.left(12));
                stream.setByteOrder(QDataStream::LittleEndian);
                stream >> frameCount;
                stream >> frameWidth;
                stream >> frameHeight;
                expectedFrameSize = frameCount * frameWidth * frameHeight * 4;
                buffer.remove(0, 12);

                qDebug() << "Expecting" << frameCount << "frames of size"
                         << frameWidth << "x" << frameHeight;
            } else {
                break;
            }
        }

        // Step 2: Wait for full frame batch
        if (expectedFrameSize >= 0 && buffer.size() >= expectedFrameSize) {
            QByteArray rawFrames = buffer.left(expectedFrameSize);
            buffer.remove(0, expectedFrameSize);
            expectedFrameSize = -1;

            emit rawFrameDataReceived(rawFrames, frameCount, frameWidth, frameHeight);
        } else {
            break;
        }
    }
}
