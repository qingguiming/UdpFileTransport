#include "recvfilehandler.h"
#include "util.h"
#include <QHostAddress>
#include <QTimer>
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QTime>

RecvFileHandler::RecvFileHandler(QObject *parent) : QObject(parent)
{
    _recvState = Recv_end;
    _currentFileSeq = 1;
    _noDataCount = 0;
    _file = nullptr;
    _isRecvMetaInfo = false;

    _pMetaSocket = new UdpSocket();
    connect(_pMetaSocket,SIGNAL(signalRecvData(QString,QByteArray)),this,SLOT(recvMetaInfo(QString,QByteArray)));

    _pMetaAckSocket = new UdpSocket();

    _pFileDataSocket = new UdpSocket();
    connect(_pFileDataSocket,SIGNAL(signalRecvData(QString,QByteArray)),this,SLOT(recvFilePkgData(QString,QByteArray)));

    _pFileDataAckSocket = new UdpSocket();

    _pFileSucessSocket = new UdpSocket();

    _pMetaSocket->bind(QHostAddress(_srcIp),METTAINFO_DSTPORT);
    _pMetaAckSocket->bind(QHostAddress(_srcIp),METTAACK_SRCPORT);
    _pFileDataSocket->bind(QHostAddress(_srcIp),FILE_PKG_DSTPORT);
    _pFileDataAckSocket->bind(QHostAddress(_srcIp),FILE_PKG_ACK_SRCPORT);
    _pFileSucessSocket->bind(QHostAddress(_srcIp),FILE_SUC_SRCPORT);

    //文件包ack 定时器
    _filePkgAckTimer = new QTimer();
    _filePkgAckTimer->setInterval(500);
    connect(_filePkgAckTimer,SIGNAL(timeout()),this,SLOT(filePkgTimeout()));

}

void RecvFileHandler::recvMetaInfo(QString srcipS,QByteArray dataArray)
{
    //源ip	目的ip	文件唯一标识	文件大小	分包大小	分包数	文件名	Md5	保留字
    //4 	4       4           4       2       4       128     32	64
    int srcip = 0;
    int dstip = 0;
    int fileNum = 0;
    int fileSize = 0;
    unsigned short pkgSize = 0;
    int pkgCount = 0;
    QString fileName ;
    char md5[MD5_SZIE] = {0};
    QString srcipStr;
    QString dstipStr;


    if(dataArray.size() < MATEINFO_SIZE)
    {
        qDebug("recvMetaInfo data len not match,data length:%d ",dataArray.size());
        return;
    }

    Util util;

    int offset = 0;
    //取原地址
    util.BytesToInt(dataArray.data() + offset,&srcip);
    srcipStr = QHostAddress(srcip).toString();
    offset = offset + 4;
    //目标地址
    util.BytesToInt(dataArray.data() + offset,&dstip);
    dstipStr = QHostAddress(dstip).toString();

    offset = offset + 4;
    //文件唯一标识
    util.BytesToInt(dataArray.data() + offset,&fileNum);
    offset = offset + 4;
    //文件大小
    util.BytesToInt(dataArray.data() + offset,&fileSize);
    offset = offset + 4;
    //分包大小
    util.BytesToShort(dataArray.data() + offset,&pkgSize);
    offset = offset + 2;
    //分包数
    util.BytesToInt(dataArray.data() + offset,&pkgCount);
    offset = offset + 4;
    //文件名
    fileName = QString(dataArray.data() + offset);
    offset = offset + FILENAME_SZIE;
    //md5
    memcpy(md5,dataArray.data() + offset,MD5_SZIE);

    qDebug("recv meta srcIP:%d dstIP:%d src:%s dst:%s",srcip,dstip,srcipStr.toStdString().data(),dstipStr.toStdString().data());
    //
    qDebug("recv meta fileNum:%d fileSize:%d pkgSize:%d pkgCount:%d",fileNum,fileSize,pkgSize,pkgCount);
    //
    qDebug("recv meta fileName:%s state:%d ",fileName.toStdString().data(),_recvState);
    qDebug()<< "recv meta time:"<<QTime::currentTime();
    //
    qDebug()<<dataArray.mid(MATEINFO_SIZE - MD5_SZIE - 64,MD5_SZIE).toHex();


    if(_recvState == Recv_end)//处于空闲态
    {
        _srcIp = srcipStr;
        _recvState = Recv_ing;
        _currentNum = fileNum;
        _currentRecvSrcIP = srcipStr;
        _isRecvMetaInfo = true;

        _metainfo.srcIP = srcipStr;
        _metainfo.dstIP = dstipStr;
        _metainfo.fileSeqnum = fileNum;
        _metainfo.fileSize = fileSize;
        _metainfo.pcakgaeSize = pkgSize;
        _metainfo.packageCount = pkgCount;
        _metainfo.fileName = fileName;
        memcpy(_metainfo.md5,md5,MD5_SZIE);
        //重置计数器
        _noDataCount = 0;
        //发送文件元信息ack
        sendMetaAck();
        //开启定时器
        _filePkgAckTimer->start();

    }
    //符合当前文件接收的元信息
    else if(_recvState != Recv_end && _currentRecvSrcIP == srcipStr && fileNum == _currentNum)
    {
        _isRecvMetaInfo = true;

        _srcIp = srcipStr;
        _metainfo.srcIP = srcipStr;
        _metainfo.dstIP = dstipStr;
        _metainfo.fileSeqnum = fileNum;
        _metainfo.fileSize = fileSize;
        _metainfo.pcakgaeSize = pkgSize;
        _metainfo.packageCount = pkgCount;
        _metainfo.fileName = fileName;
        memcpy(_metainfo.md5,md5,MD5_SZIE);

        //重置计数器
        _noDataCount = 0;

        //发送文件元信息ack
        sendMetaAck();
        if(_file && (_file->size() == _metainfo.fileSize))
        {
            //md5 校验文件
            checkFile();

        }
    }




}

void RecvFileHandler::recvFilePkgData(QString srcip, QByteArray dataArray)
{
    //qDebug()<<"recvFilePkgData len"<<dataArray.size();

    //文件唯一标识	文件包序号	数据区长度	数据区
    //4             4           2           n
    int dataSize = dataArray.size();

    if(dataSize < 10)
    {
        qDebug("recvFilePkgData data len error");
        return;
    }

    int fileSeqNum = 0;
    int pkgNum = 0;
    unsigned short pkgSize = 0;
    Util util;
    int offset = 0;

    //获取文件序列号
    util.BytesToInt(dataArray.data(),&fileSeqNum);
    offset = offset + 4;
    //获取文件包号
    util.BytesToInt(dataArray.data() + offset,&pkgNum);
    offset = offset + 4;
    //获取数据长度
    util.BytesToShort(dataArray.data() + offset,&pkgSize);
    offset = offset + 2;

    QByteArray fileArray;
    if(_recvState != Recv_end && fileSeqNum == _currentNum && srcip == _currentRecvSrcIP)
    {
        fileArray = dataArray.mid(offset,pkgSize);
    }

#ifdef SEND_DATA    //发端目前不会发送元信息后立马发送文件包
    else if(_recvState == Recv_end)//接收到第一包（未收到元信息的情况下）
    {
        fileArray = dataArray.mid(offset,pkgSize);

        _recvState = Recv_ing;
        _currentNum = fileSeqNum;
        _currentRecvSrcIP = srcip;
        _srcIp = srcip;
        //开启定时器
        _filePkgAckTimer->start();

    }
#endif
    else
    {
        return;
    }

    //将数据添加到滑动窗口
    _noDataCount = 0;
    addDataToWindow(fileArray,pkgNum);


}

void RecvFileHandler::filePkgTimeout()
{
    if(_noDataCount == 5)
    {
        resetState();
        return;
    }

    _noDataCount++;
    sendFilePkgAck();
}

void RecvFileHandler::sendMetaAck()
{
    //源ip	目的ip	文件唯一标识
    //4     4       4
    //qDebug("sendMetaAck srcIP:%s dstIP:%s fileSeq:%d  ",_metainfo.srcIP.toUtf8().data(),
     //      _metainfo.dstIP.toUtf8().data(),_metainfo.fileSeqnum);


    int srcIp = QHostAddress(_metainfo.srcIP).toIPv4Address();
    int dstIp = QHostAddress(_metainfo.dstIP).toIPv4Address();
    unsigned int fileSeq = _metainfo.fileSeqnum;

    Util util;
    QByteArray dataArray(12,0);
    //源ip
    util.intToBigByteArray(srcIp,dataArray.data());
    //目标ip
    util.intToBigByteArray(dstIp,dataArray.data() + 4);
    //唯一标志符
    util.intToBigByteArray(fileSeq,dataArray.data() + 8);

    //发送数据
    //qDebug()<<"sendMetaAck" << _srcIp;
    _pMetaAckSocket->sendData(dataArray,QHostAddress(_srcIp),METTAACK_DSTPORT);



}

void RecvFileHandler::sendFilePkgAck()
{
    //qDebug()<<"sendFilePkgAck ";
    if(_isRecvMetaInfo == false) return;

    int minSeq = 0;
    int maxSeq = 0;

    if(_slidWindow.size() > 0)
    {
         minSeq = _slidWindow.first().index;
         maxSeq = _slidWindow.last().index;
    }
    else
    {
        //表示缓冲区数据写完、通知发端更新发端滑动窗口
        minSeq = _currentFileSeq;
        maxSeq = _currentFileSeq;

    }

    int windowSize = _slidWindow.size();
    int breakCondition = windowSize - 1;
    int len = 0;

    char val = 0;
    QByteArray bitArray;
    for(int i = 0; i < windowSize;i++)
    {
        len ++;
        int offset = i % 8;
        if(_slidWindow.at(i).recvAck != 3)
        {
            val = val << 1 ;
        }
        else
        {
            val = (val << 1) | 0x01;

        }


        if(i == breakCondition || i == ACKBUF_BIT_MAX)
        {
            bitArray.append(val);
            break;

        }

        if(offset == 7)
        {
            bitArray.append(val);
            val = 0;
        }

    }

    qDebug()<<"sendFilePkgAck --len min max"<<len << minSeq<< maxSeq << QTime::currentTime();
    QByteArray ackArray(21+bitArray.size(),0);
    int offset = 0;
    Util util;

    //文件唯一标识
    util.intToBigByteArray(_currentNum,ackArray.data() + offset);
    offset = offset + 4;

    //mode
    ackArray[offset] = ACK_RECVSEQ_BIT_MODE;
    offset = offset + 1;
    //minSeq
    util.intToBigByteArray(minSeq,ackArray.data() + offset);
    offset = offset + 4;
    //maxSeq
    util.intToBigByteArray(maxSeq,ackArray.data() + offset);
    offset = offset + 4;
    //recv package
    util.intToBigByteArray(0,ackArray.data() + offset);
    offset = offset + 4;
    //len
    util.intToBigByteArray(len,ackArray.data() + offset);
    offset = offset + 4;
    //data
    memcpy(ackArray.data() + offset,bitArray.data(),bitArray.size());

    _pFileDataAckSocket->sendData(ackArray,QHostAddress(_srcIp),FILE_PKG_ACK_DSTPORT);


}

void RecvFileHandler::sendFileRecvSucess(bool recvResult)
{
    //文件序列号     结果(0:false 1:true)
    //4             4
    QByteArray dataArray(8,0);
    Util util;

    int result = 0;
    if(recvResult)
    {
        result = 1;
    }

    //文件序列号
    util.intToBigByteArray(_currentNum,dataArray.data());
    //接收结果
    util.intToBigByteArray(result,dataArray.data() + 4);

    //发送数据
    _pFileSucessSocket->sendData(dataArray,QHostAddress(_srcIp),FILE_SUC_DSTPORT);

}

void RecvFileHandler::addDataToWindow(const QByteArray &data, unsigned int pkgNum)
{
    PackageFrame tempPkgFrame;
    tempPkgFrame.index = pkgNum;
    tempPkgFrame.frameArray = data;
    tempPkgFrame.recvAck = 3;

    //qDebug("RecvFileHandler::currentNum:%d pkgNum:%d size:%d %d",_currentFileSeq,pkgNum,_slidWindow.size(),_recvState);

    if(_slidWindow.size() > SLIDWINDOW_MAXSIZE)
    {
        qDebug("RecvFileHandler::addDataToWindow window full %d %d state:%d",_slidWindow.size(),pkgNum,_recvState);
        qDebug()<<"RecvFileHandler::addDataToWindow"<<QTime::currentTime();
        return;
    }

    if(pkgNum < _currentFileSeq)
    {
        //qDebug("RecvFileHandler::addDataToWindow window invaild data %d %d ",pkgNum,_currentFileSeq);
        return;
    }

    if(_slidWindow.size() == 0)
    {
        if(_currentFileSeq == pkgNum)//前面的包都成功写入
        {
            _slidWindow.append(tempPkgFrame);
        }
        else if(_currentFileSeq < pkgNum)//前面的包还有没收到
        {
            int interval = pkgNum - _currentFileSeq;
            for(int i = 0; i < interval;i++)
            {
                PackageFrame pkgFrame;
                pkgFrame.index = _currentFileSeq + i;
                _slidWindow.append(pkgFrame);

            }
            _slidWindow.append(tempPkgFrame);


        }
        else
        {
            return;
        }
    }



    if(_slidWindow.last().index < pkgNum)//添加数据
    {
        int lastIndex = _slidWindow.last().index;
        int interval = pkgNum - lastIndex;
        for(int i = 1; i < interval;i++)
        {
            PackageFrame pkgFrame;
            pkgFrame.index = lastIndex + i;
            _slidWindow.append(pkgFrame);


        }
        _slidWindow.append(tempPkgFrame);
    }
    else
    {
        int lastIndex = _slidWindow.last().index;
        int interval = lastIndex - pkgNum;
        if(interval < 0) return;
       // qDebug()<<"QList " << _slidWindow.size() << interval << lastIndex << pkgNum << _slidWindow.first().index << _currentFileSeq;
        _slidWindow[_slidWindow.size() - 1 - interval].frameArray = data;
        _slidWindow[_slidWindow.size() - 1 - interval].recvAck = 3;

    }

    //将已经连续的接收的数据包的写入内存
    int count = 0;
    QList<PackageFrame>::iterator endIterator;
    for(auto i = _slidWindow.begin() ; i != _slidWindow.end(); i++)
    {
        if( (*i).recvAck == 3)
        {
            endIterator = i;
            _currentFileSeq++;
            if(!_file)
            {
                QString oldFileName = QCoreApplication::applicationDirPath() + RECV_TEMP_FILENAME;
                _file = new QFile(oldFileName);
                _file->remove();
                _file->open(QIODevice::WriteOnly);
            }
            _file->write((*i).frameArray);
            //qDebug()<<"write len " <<_file->write((*i).frameArray) <<pkgNum << (*i).index <<  _slidWindow.size();
        }
        else
        {
            break;
        }
        count++;
    }

    if(count > 0)
    {
        _slidWindow.erase(_slidWindow.begin(),_slidWindow.begin() + count);
        //qDebug()<<"_slidWindow earse -- "<<_slidWindow.size();

    }

    //已经接收到元信息
    if(_isRecvMetaInfo && _currentFileSeq > _metainfo.packageCount)
    {
        if(_file)
        {
            _file->close();
        }
         //校验文件
        checkFile();
    }


}

void RecvFileHandler::checkFile()
{
    //计算文件md5值
    QCryptographicHash hash(QCryptographicHash::Md5);//实例化一个计算md5的对象

    QString oldFileName = QCoreApplication::applicationDirPath() + RECV_TEMP_FILENAME;

    _file = new QFile(oldFileName);

    bool openRet = _file->open(QIODevice::ReadOnly);

    if(!openRet)
    {
        qDebug("checkFile open failed");
        return;
    }


    while(!_file->atEnd())//循环直到读完
    {
        QByteArray content = _file->read(10*1024*1024);//一次性读取10MB
        hash.addData(content);//添加数据
    }
    _file->close();

    QByteArray md5Array = hash.result();//取出结果


    qDebug("calc md5:%s",md5Array.toHex().data());

    QByteArray metaMd5(MD5_SZIE,0);
    memcpy(metaMd5.data(),_metainfo.md5,MD5_SZIE);
    qDebug("meta md5:%s",metaMd5.toHex().data());

    bool md5Result = true;

    for(int i = 0;i < MD5_SZIE;i++)
    {
        if(metaMd5[i] != md5Array[i])
        {
           md5Result = false;
           break;
        }
    }



    //md5 验证通过，重命名
    if(md5Result)
    {
        renameRecvName();
    }
    else
    {
        qDebug()<<"checkFile md5 failed";
    }
    //发送文件接收结果
    sendFileRecvSucess(md5Result);
    //重置状态
    resetState();


}

void RecvFileHandler::renameRecvName()
{

    QString oldFileName = QCoreApplication::applicationDirPath() + RECV_TEMP_FILENAME;
    QString newFileName = QCoreApplication::applicationDirPath() + QString("/%1").arg(_metainfo.fileName);

    qDebug()<<"renameRecvName" << oldFileName << newFileName;

    std::rename(oldFileName.toStdString().data(),newFileName.toStdString().data());


}

void RecvFileHandler::resetState()
{
    _slidWindow.clear();

    if(_file && _file->isOpen())
    {
        _file->close();
    }
    delete _file;
    _file = nullptr;

    _srcIp = "";
    _currentNum = 0;
    _currentFileSeq = 1;
    _noDataCount = 0;
    _recvState = Recv_end;
    _currentRecvSrcIP = "";
    _isRecvMetaInfo = false;
    _filePkgAckTimer->stop();



}


