#ifndef RECVFILEHANDLER_H
#define RECVFILEHANDLER_H

#include <QObject>
#include <QList>
#include <QMap>
#include "udpsocket.h"
#include "commondefine.h"
#include <QFile>
#include <QTimer>

//源ip	目的ip	文件唯一标识	文件大小	分包大小	分包数	文件名	Md5	保留字
//4 	4       4           4       2       4       128     32	64
struct strFileMeta
{
    QString srcIP;
    QString dstIP;
    unsigned int fileSeqnum;
    unsigned int fileSize;
    unsigned short pcakgaeSize;
    unsigned int packageCount;
    QString fileName;
    char md5[MD5_SZIE];
};

class RecvFileHandler : public QObject
{
    Q_OBJECT
    enum RecvState
    {
        Recv_end = 1,//接收空闲态
        Recv_ing = 2,//接收中
        Recv_waitMeta = 3,//文件包接收完成、等待元信息
    };
public:
    explicit RecvFileHandler(QObject *parent = nullptr);



signals:

public slots:
    //接收文件元信息
    void recvMetaInfo(QString srcip,QByteArray dataArray);
    //接收文件数据包
    void recvFilePkgData(QString srcip,QByteArray dataArray);
    //文件包接收ack 定时器
    void filePkgTimeout();

private:
    //返回文件元信息ack
    void sendMetaAck();
    //返回文件包ack
    void sendFilePkgAck();
    //发送文件接收成功指令
    void sendFileRecvSucess(bool recvResult);
    //将数据添加到滑动窗口
    void addDataToWindow(const QByteArray& data,unsigned int pkgNum);
    //校验文件以及返回结果
    void checkFile();
    //重命名接收的文件
    void renameRecvName();
    //重置状态
    void resetState();

private:
    //滑动窗口
    QList<PackageFrame> _slidWindow;
    QFile *_file;
    QString _srcIp;
    int _currentNum;//当前文件唯一标识
    int _currentFileSeq;//文件包当前最小序号
    int _noDataCount;//无数据接收计数器
    strFileMeta _metainfo;
    RecvState _recvState;
    QString _currentRecvSrcIP;
    bool _isRecvMetaInfo;
    QTimer *_filePkgAckTimer;

    UdpSocket *_pMetaSocket;//元信息socket
    UdpSocket *_pMetaAckSocket;//元信息ack socket
    UdpSocket *_pFileDataSocket;//文件包socket
    UdpSocket *_pFileDataAckSocket;//文件包ack socket
    UdpSocket *_pFileSucessSocket;//

};

#endif // RECVFILEHANDLER_H
