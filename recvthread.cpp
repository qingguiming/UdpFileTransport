#include "recvthread.h"

RecvThread::RecvThread(QObject *parent) : QThread(parent)
{
    _recvHandler = nullptr;
}

void RecvThread::run()
{
    _recvHandler = new RecvFileHandler();
    this->exec();
}
