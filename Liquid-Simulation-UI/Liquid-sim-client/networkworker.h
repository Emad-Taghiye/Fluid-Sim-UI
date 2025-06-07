#ifndef NETWORKWORKER_H
#define NETWORKWORKER_H

#include <QObject>
#include <QTcpSocket>
#include <QImage>
#include <QElapsedTimer>
#include <QQueue>
#include <QTimer>


class NetworkWorker : public QObject
{
    Q_OBJECT
public:
    explicit NetworkWorker(QObject *parent = nullptr);
    ~NetworkWorker();

    void connectToServer(const QString &host, quint16 port);
    void sendCommand(const QString &command);

signals:
    void newFrame(QImage img);
    void connected();
    void disconnected();
    void errorOccurred(QString error);
    void rawFrameDataReceived(QByteArray rawData, int frameCount, int width, int height);

private slots:
    void onReadyRead();
    void onConnected();
    void onDisconnected();
    void onError(QAbstractSocket::SocketError socketError);


private:
    QTcpSocket *socket;
    qint32 expectedFrameSize = -1;  // How many bytes we expect for the current frame
    QByteArray buffer;              // Buffer to hold incoming data
    QElapsedTimer frameTimer;      // ✅ Already there
    QImage pendingFrame;           // ✅ Add this
    bool framePending = false;
    int frameWidth = -1;
    int frameHeight = -1;
    int frameCount = 0;
    QQueue<QImage> pendingFrames;
    QTimer *frameDispatchTimer;

};

#endif // NETWORKWORKER_H
