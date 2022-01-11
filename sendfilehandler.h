#ifndef SENDFILEHANDLER_H
#define SENDFILEHANDLER_H

#include <QThread>
#include <QFile>
#include <QList>
#include <QTimer>
#include "udpsocket.h"
#include "commondefine.h"
#include <QTime>



struct FileBusiness
{
    QString srcIP;
    QString dstIP;
    QString filePath;
};

class SendFileHandler : public QObject
{
    Q_OBJECT
    enum SendState
    {
        Send_end = 1,
        Send_ing = 2,
        FILE_READ_END = 3,
    };
public:
    explicit SendFileHandler(QObject *parent = nullptr);

    void setDeviceIp(QString srcIP,QString dstIP);

private:
    void sendFileMetaInfo(QString srcIp,QString dstIp,QString filepath);
    void sendFilePackage();

    void resetFileState();
    void setFileFinishedState();//设置文件发送结束状态，不论发送成功与否
    void updateSliderWindow(int index);
    void addDataToSliderWindow();
    void markRecvState(int mode,int minIndex,int arrayLen,QByteArray& stateArray);


private:
    QList<PackageFrame> _slidWindow;
    QFile *_file;
    QString _srcIp;
    QString _dstIp;
    QByteArray _mateInfoArray;
    int _currentNum;
    int _currentFileSeq;
    int _currentMinIndex;//为第一个没有收到的索引
    QTimer *_metaTimer;
    QTimer *_maxAckTimer;
    SendState _currentState;
    QList<FileBusiness> _businessCaches;

    QTime _startTime;
    QTime _endTime;


    UdpSocket *_pMetaSocket;//元信息socket
    UdpSocket *_pMetaAckSocket;//元信息ack socket
    UdpSocket *_pFileDataSocket;//文件包socket
    UdpSocket *_pFileDataAckSocket;//文件包ack socket
    UdpSocket *_pFileSucessSocket;//




private slots:
    void onMetaTimeout();
    void onMaxAckTimeout();




    void recvMeteAck(QString srcip,QByteArray data);
    void recvPackageAck(QString srcip,QByteArray data);
    void recvFileIndict(QString srcip,QByteArray data);






signals:

public slots:
    void onSendFile(QString srcIp,QString dstIp,QString filepath);

};

#endif // SENDFILEHANDLER_H
