#include "sendfilehandler.h"
#include <QFile>
#include <QDebug>
#include <QHostAddress>
#include <QFileInfo>
#include "commondefine.h"
#include <QCryptographicHash>
#include <util.h>
#include <QTime>

SendFileHandler::SendFileHandler(QObject *parent) : QObject(parent)
{
    _file = nullptr;
    _currentNum = 0;
    _currentState = Send_end;

    _pMetaSocket = new UdpSocket();
    //connect(_pMetaSocket,SIGNAL(signalRecvData(QByteArray)),this,SLOT(recvMeteAck(QByteArray)));

    _pMetaAckSocket = new UdpSocket();
    connect(_pMetaAckSocket,SIGNAL(signalRecvData(QString,QByteArray)),this,SLOT(recvMeteAck(QString,QByteArray)));

    _pFileDataSocket = new UdpSocket();
    //connect(_pMetaSocket,SIGNAL(signalRecvData(QByteArray)),this,SLOT());

    _pFileDataAckSocket = new UdpSocket();
    connect(_pFileDataAckSocket,SIGNAL(signalRecvData(QString,QByteArray)),this,SLOT(recvPackageAck(QString,QByteArray)));

    _pFileSucessSocket = new UdpSocket();
    connect(_pFileSucessSocket,SIGNAL(signalRecvData(QString,QByteArray)),this,SLOT(recvFileIndict(QString,QByteArray)));

    _pMetaSocket->bind(QHostAddress(_srcIp),METTAINFO_SRCPORT);
    _pMetaAckSocket->bind(QHostAddress(_srcIp),METTAACK_DSTPORT);
    _pFileDataSocket->bind(QHostAddress(_srcIp),FILE_PKG_SRCPORT);
    _pFileDataAckSocket->bind(QHostAddress(_srcIp),FILE_PKG_ACK_DSTPORT);
    _pFileSucessSocket->bind(QHostAddress(_srcIp),FILE_SUC_DSTPORT);

    _metaTimer = new QTimer();
    _metaTimer->setInterval(1000);
    connect(_metaTimer,SIGNAL(timeout()),this,SLOT(onMetaTimeout()));


    _maxAckTimer = new QTimer();
    _maxAckTimer->setInterval(10000);
    connect(_maxAckTimer,SIGNAL(timeout()),this,SLOT(onMaxAckTimeout()));


}

void SendFileHandler::setDeviceIp(QString srcIP, QString dstIP)
{
    _srcIp = srcIP;
    _dstIp = dstIP;





}

void SendFileHandler::sendFileMetaInfo(QString srcIp, QString dstIp, QString filepath)
{
    onSendFile(srcIp,dstIp,filepath);
}

void SendFileHandler::onSendFile(QString srcIp, QString dstIp, QString filepath)
{
    _startTime = QTime::currentTime();

    if(srcIp.size() <= 0 || dstIp.size() <= 0  || filepath.size() <= 0)
    {
        qDebug("sendFile para error");
        return;
    }
    //?????????????????????????????????
   if(Send_end != _currentState)
   {
       FileBusiness business;
       business.dstIP = dstIp;
       business.srcIP = srcIp;
       business.filePath = filepath;

       _businessCaches.append(business);
       qDebug("sendFile _currentState not finished");

        return;
   }

    //??????????????????????????????
    if(_file)
    {
       _file->close();
       delete _file;
       _file = nullptr;
    }

    //????????????????????????????????????????????????
    _file = new QFile(filepath);
    if(!_file->exists())
    {
        qDebug("sendfile not exists,fileName:%s ",filepath.toUtf8().data());
        return;
    }


    if(!_file->open(QIODevice::ReadOnly))
    {
        qDebug("open file failed  \n");
        return;
    }

    //????????????????????????
    resetFileState();


    //?????????????????????
    int  nFileSize = _file->size();
    int nPackageCount = nFileSize / FILE_PACKAGE_SIZE;


    if(nFileSize % FILE_PACKAGE_SIZE != 0)
    {
        nPackageCount++;
    }
    qDebug("fileSize:%d packageSize:%d pcakageCount:%d ",nFileSize,FILE_PACKAGE_SIZE,nPackageCount);


    //???????????????
    _currentNum++;
    qDebug("_currentNum:%d",_currentNum);

    //????????????md5???
    QCryptographicHash hash(QCryptographicHash::Md5);//?????????????????????md5?????????
    while(!_file->atEnd())//??????????????????
    {
        QByteArray content = _file->read(10*1024*1024);//???????????????10MB
        hash.addData(content);//????????????
    }
    //??????????????????????????????????????????
    _file->seek(0);

    QByteArray md5Array = hash.result();//????????????
    qDebug("md5:%s",md5Array.toHex().data());

    //?????????
    QByteArray fileNameArray(128,0);

    QFileInfo fileInfo(filepath);

    fileNameArray = fileInfo.fileName().toUtf8();

    //
    QHostAddress srcAddr(srcIp);
    QHostAddress dstAddr(dstIp);

    _srcIp = srcIp;
    _dstIp = dstIp;

    qDebug("srcIP:%d  dstIP:%d ",srcAddr.toIPv4Address(),dstAddr.toIPv4Address());

    _mateInfoArray.clear();
    //???ip	??????ip	??????????????????	????????????	????????????	?????????	?????????	Md5     ?????????
    //4     4       4           4       2       4       128     32      64
    _mateInfoArray.resize(MATEINFO_SIZE);
    memset(_mateInfoArray.data(),0,MATEINFO_SIZE);

    Util util;

    int offset = 0;
    util.intToBigByteArray(srcAddr.toIPv4Address(),_mateInfoArray.data() + offset);
    offset += 4;
    util.intToBigByteArray(dstAddr.toIPv4Address(),_mateInfoArray.data() + offset);
    offset += 4;
    util.intToBigByteArray(_currentNum,_mateInfoArray.data() + offset);
    offset += 4;
    util.intToBigByteArray(nFileSize,_mateInfoArray.data() + offset);
    offset += 4;
    util.shortToBigByteArray(FILE_PACKAGE_SIZE,_mateInfoArray.data() + offset);
    offset += 2;
    util.intToBigByteArray(nPackageCount,_mateInfoArray.data() + offset);
    offset += 4;
    memcpy(_mateInfoArray.data() + offset,fileNameArray.data(),fileNameArray.size());
    offset += FILENAME_SZIE;
    memcpy(_mateInfoArray.data() + offset,md5Array.data(),md5Array.size());
    offset += MD5_SZIE;


    //???????????????
    qDebug()<<"_dstIp" <<_dstIp;
    _pMetaSocket->sendData(_mateInfoArray,QHostAddress(_dstIp),METTAINFO_DSTPORT);
    _metaTimer->start();
    _maxAckTimer->start();

}

void SendFileHandler::sendFilePackage()
{

    //???????????????????????????
    addDataToSliderWindow();
    //???????????????????????????

    for(int i = 0; i < _slidWindow.size() ; i++)
    {
        _pFileDataSocket->sendData(_slidWindow[i].frameArray,QHostAddress(_dstIp),FILE_PKG_DSTPORT);

        //??????????????????udp??????
        if(i % 5 == 0)
        {
            QThread::msleep(1);
        }
    }



}

void SendFileHandler::recvMeteAck(QString srcip,QByteArray data)
{
    //???ip	??????ip	??????????????????
    //4     4           4
    if(data.size() < 12)
    {
        qDebug("recvMeteAck len error");
        return;
    }

    int ackSeq = 0;
    Util util;
    util.BytesToInt(data.data() + 8,&ackSeq);
    //qDebug("recvMeteAck seq:%d",ackSeq);
    if(ackSeq == _currentNum)
    {
        if(_metaTimer->isActive())
        {
            _metaTimer->stop();
            qDebug("recvMeteAck timer stop ");
            //???????????????
            sendFilePackage();
        }



    }
    else
    {
        qDebug("recvMeteAck not macth seq:%d _currentNum:%d ",ackSeq,_currentNum);

    }
}

void SendFileHandler::recvPackageAck(QString srcip,QByteArray data)
{
    //??????????????????	Mode	???????????????	???????????????	???????????????	??????????????????	??????
    //4             1           4           4           4           4       N

    if(data.size() < 21)
    {
        qDebug("recvPackageAck len error");
        return;
    }

    int ackSeq = 0;
    Util util;
    util.BytesToInt(data.data(),&ackSeq);
    //qDebug("recvMeteAck seq:%d",ackSeq);
    if(ackSeq != _currentNum)
    {
        qDebug("recvPackageAck not macth seq:%d _currentNum:%d ",ackSeq,_currentNum);
        return;
    }

    char mode = data[4];
    int minIndex = 0;
    int maxIndex = 0;
    int recvPackageCount = 0;
    int ackArraylen = 0;

    util.BytesToInt(data.data() + 5,&minIndex);
    util.BytesToInt(data.data() + 9,&maxIndex);
    util.BytesToInt(data.data() + 13,&recvPackageCount);
    util.BytesToInt(data.data() + 17,&ackArraylen);

    qDebug("min:%d max:%d recvCount:%d ackNum:%d ",minIndex,maxIndex,recvPackageCount,ackArraylen);


    if(minIndex > maxIndex)
    {
        qDebug("recvPackageAck error,minIndex > maxIndex");
        return;
    }

    //
    if(_slidWindow.first().index < minIndex)//????????????????????????
    {
        //??????????????????
        updateSliderWindow(minIndex - 1);
    }

    qDebug()<<"recvPackageAck "<<_slidWindow.first().index <<minIndex << maxIndex;
    //?????????????????????????????????
    QByteArray contentArray = data.mid(21);
    markRecvState(mode,minIndex,ackArraylen,contentArray);

    //???????????????????????????
    if(_currentState == Send_ing)
    {
        addDataToSliderWindow();
    }


    //???????????????????????????(??????????????????????????????????????????)
    int nSecStartIndex = maxIndex - minIndex;
    int windowSize = _slidWindow.size();
    for(int i = 0; i < windowSize ; i++)
    {
        //????????????ack?????????????????????
        if(i >= nSecStartIndex)
        {
            _pFileDataSocket->sendData(_slidWindow[i].frameArray,QHostAddress(_dstIp),FILE_PKG_DSTPORT);
            continue;
        }
        //????????????ack???????????????
        if(_slidWindow[i].recvAck == 2)
        {
            _pFileDataSocket->sendData(_slidWindow[i].frameArray,QHostAddress(_dstIp),FILE_PKG_DSTPORT);

        }
    }

    _maxAckTimer->start();



}

void SendFileHandler::recvFileIndict(QString srcip,QByteArray data)
{
    //??????????????????    result???0:failed 1:success???
    //4                4
    if(data.size() < 8)
    {
        qDebug("recvFileIndict len error");
        return;
    }

    int ackSeq = 0;
    Util util;
    util.BytesToInt(data.data(),&ackSeq);
    qDebug("recvFileIndict seq:%d",ackSeq);
    if(ackSeq == _currentNum)
    {
        setFileFinishedState();
    }
    else
    {
        qDebug("recvFileIndict not macth seq:%d _currentNum:%d ",ackSeq,_currentNum);

    }


}

void SendFileHandler::resetFileState()
{
    _currentFileSeq = 1;
    _currentState = Send_ing;

}

void SendFileHandler::setFileFinishedState()
{
    _endTime = QTime::currentTime();
    qDebug()<<"setFileFinishedState start "<< _startTime << " end time" << _endTime;
    _currentFileSeq = 1;
    _currentState = Send_end;
    _slidWindow.clear();
    if(_file)
    {
        _file->close();
        delete(_file);
        _file = nullptr;
    }
    //???????????????
    if(_metaTimer->isActive()) _metaTimer->stop();
    if(_maxAckTimer->isActive()) _maxAckTimer->stop();

    //???????????????????????????
    if(_businessCaches.size() > 0)
    {
        sendFileMetaInfo(_businessCaches.at(0).srcIP,_businessCaches.at(0).dstIP
                         ,_businessCaches.at(0).filePath);
        _businessCaches.removeFirst();
    }


}

void SendFileHandler::updateSliderWindow(int index)
{
    int count = 0;
    QList<PackageFrame>::iterator endIterator;
    for(auto i = _slidWindow.begin() ; i != _slidWindow.end(); i++)
    {
        if( (*i).index == index)
        {
            endIterator = i;
            break;
        }
        count++;
    }

    _slidWindow.erase(_slidWindow.begin(),endIterator);

}

void SendFileHandler::addDataToSliderWindow()
{
    Util util;

    while(_slidWindow.size() < SLIDWINDOW_MAXSIZE && _currentState != FILE_READ_END)
    {
        QByteArray fileContentArray = _file->read(FILE_PACKAGE_SIZE);
        if(fileContentArray.size() == FILE_PACKAGE_SIZE)
        {

        }
        else if(fileContentArray.size() == 0)
        {
            _currentState = FILE_READ_END;
            _file->close();
            return;
        }
        else
        {
            _file->close();
            _currentState = FILE_READ_END;

        }

        //??????????????????	???????????????	???????????????	?????????
        //4             4           2           n


        QByteArray contentFrame(fileContentArray.size() + 10,0);
        util.intToBigByteArray(_currentNum,contentFrame.data());
        util.intToBigByteArray(_currentFileSeq,contentFrame.data() + 4);
        util.shortToBigByteArray((unsigned short)fileContentArray.size(),contentFrame.data() + 8);
        memcpy(contentFrame.data() + 10,fileContentArray.data(),fileContentArray.size());

        PackageFrame tempPackagFrame;
        tempPackagFrame.index = _currentFileSeq;
        tempPackagFrame.frameArray = contentFrame;
        _slidWindow.append(tempPackagFrame);
        _currentFileSeq++;


    }

}

void SendFileHandler::markRecvState(int mode, int minIndex, int arrLen,QByteArray &stateArray)
{
    Util util;
    int arraySize = stateArray.size();
    if(mode == ACK_RECVSEQ_MODE)
    {
        if(arrLen * 4 != arraySize)
        {
            qDebug("markRecvState stateArray size error");
            return;
        }

        for(int i = 0;i < arrLen;i++)
        {
            int tempIndex = 0;
            util.BytesToInt(stateArray.data() + i*4,&tempIndex);
            _slidWindow[tempIndex - minIndex - 1].recvAck = ACK_RECV_STATE;
        }
    }

    if(mode == ACK_NORECVSEQ_MODE)
    {
        if(arrLen * 4 != arraySize)
        {
            qDebug("markRecvState stateArray size error");
            return;
        }

        for(int i = 0;i < arrLen;i++)
        {
            int tempIndex = 0;
            util.BytesToInt(stateArray.data() + i*4,&tempIndex);
            _slidWindow[tempIndex - minIndex - 1].recvAck = ACK_NORECV_STATE;
        }
    }

    if(mode == ACK_RECVSEQ_BIT_MODE)
    {
        if(arrLen > (arraySize * 8))
        {
            qDebug("markRecvState stateArray size error %d %d ",arraySize*8,arrLen);
            return;
        }

        for(int i = 0;i < arrLen;i++)
        {
            int tempIndex = arrLen / 8;
            int modVal = arrLen % 8;

            int recvState = stateArray[tempIndex] & (0x1 << modVal);

            _slidWindow[i].recvAck = recvState == 1 ? ACK_RECV_STATE : ACK_NORECV_STATE;
        }
    }


}

void SendFileHandler::onMetaTimeout()
{
    if(Send_end != _currentState)
    {
        _pMetaSocket->sendData(_mateInfoArray,QHostAddress(_dstIp),METTAINFO_DSTPORT);

    }
}

void SendFileHandler::onMaxAckTimeout()
{
    qDebug()<<"onMaxAckTimeout ,file send failed";
    setFileFinishedState();
}

