#include "sendthread.h"

SendThread::SendThread(QObject *parent) : QThread(parent)
{
    _sendHandler = nullptr;
}



void SendThread::run()
{
    _sendHandler = new SendFileHandler();
    connect(this,SIGNAL(signaleSendFile(QString,QString,QString)),_sendHandler,SLOT(onSendFile(QString,QString,QString)));

    this->exec();
}
