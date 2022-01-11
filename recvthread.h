#ifndef RECVTHREAD_H
#define RECVTHREAD_H

#include <QThread>
#include "recvfilehandler.h"
class RecvThread : public QThread
{
    Q_OBJECT
public:
    explicit RecvThread(QObject *parent = nullptr);

signals:

public slots:

protected:
    void run();

private:
    RecvFileHandler* _recvHandler;
};

#endif // RECVTHREAD_H
