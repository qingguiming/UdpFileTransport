#ifndef SENDTHREAD_H
#define SENDTHREAD_H

#include <QThread>
#include "sendfilehandler.h"

class SendThread : public QThread
{
    Q_OBJECT
public:
    explicit SendThread(QObject *parent = nullptr);

signals:
    void signaleSendFile(QString srcIp,QString dstIp,QString filepath);


private slots:


protected:
    void run();

private:
    SendFileHandler* _sendHandler;

};

#endif // SENDTHREAD_H
