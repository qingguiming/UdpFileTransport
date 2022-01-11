#ifndef UDPSOCKET_H
#define UDPSOCKET_H

#include <QObject>
#include <QUdpSocket>

class UdpSocket : public QObject
{
    Q_OBJECT
public:
    explicit UdpSocket(QObject *parent = nullptr);
    void bind(QHostAddress addr,unsigned short port);
signals:
    void signalRecvData(QString ip,QByteArray data);

public slots:
    void sendData(QByteArray array,QHostAddress addr,int port);

private slots:
    void dataComing();

private:
    QUdpSocket *_socket;
    int _port;
    //编码
    QByteArray encodeFrame(const QByteArray &array);
    //解码
    QByteArray decodeFrame(const QByteArray &array);

};

#endif // UDPSOCKET_H
