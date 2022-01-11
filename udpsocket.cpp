#include "udpsocket.h"
#include "util.h"
#include <QDebug>

UdpSocket::UdpSocket(QObject *parent) : QObject(parent)
{
    _socket = new QUdpSocket;
    connect(_socket,&QUdpSocket::readyRead,this,&UdpSocket::dataComing);
    _port = 0;
}

void UdpSocket::bind(QHostAddress addr, unsigned short port)
{
    _port = port;
    bool bindResult = _socket->bind(QHostAddress::AnyIPv4,port);
    qDebug()<<"UdpSocket bind result" << bindResult;
}

void UdpSocket::sendData(QByteArray array, QHostAddress addr, int port)
{
    QByteArray data = encodeFrame(array);
    _socket->writeDatagram(data,addr,port);
}

void UdpSocket::dataComing()
{
    //qDebug()<<"dataComing";
    QByteArray ba;
    QHostAddress addr;
    quint16 port = 0;
    while(_socket->hasPendingDatagrams())
    {
        ba.resize(_socket->pendingDatagramSize());
        _socket->readDatagram(ba.data(), ba.size(),&addr,&port);
        //qDebug()<<ba;
        QByteArray content = decodeFrame(ba);
        if(content.size() > 0)
        {
            emit signalRecvData(addr.toString(),content);

        }
    }
}

QByteArray UdpSocket::encodeFrame(const QByteArray &array)
{
    QByteArray dataArray;
    dataArray.resize(array.size() + 6);
    Util util;

    dataArray[0] = 0xCE;

    int offset = 1;
    unsigned short dataSize = array.size();
    util.shortToBigByteArray(dataSize,dataArray.data() + offset);
    offset = offset + 2;


    unsigned short crc = util.crc16(array.data(),array.size());
    util.shortToBigByteArray(crc,dataArray.data() + offset);
    offset = offset + 2;


    memcpy(dataArray.data() + offset,array.data(),array.size());
    dataArray[array.size() + 5] = 0xCE;


    return dataArray;


}

QByteArray UdpSocket::decodeFrame(const QByteArray &array)
{
   // 标志位           数据长度        Crc校验 	有效数据区	标志位
   // 0xCE（1byte）    2字节          2字节       Len byte	0xCE（1byte）
    int size = array.size();
    if(size > 6 && (unsigned char)array.at(0) == 0xCE && (unsigned char)array.at(size - 1) == 0xCE)
    {
        return array.mid(5,size - 6);
    }
    else
    {
        QByteArray ret;
        qDebug()<<"port " << _port << "error";
        return ret;
    }

}
