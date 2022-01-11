#ifndef COMMONDEFINE_H
#define COMMONDEFINE_H
#include <QByteArray>

#define FILE_PACKAGE_SIZE 6000
#define MATEINFO_SIZE 246
#define FILENAME_SZIE 128
#define MD5_SZIE 32
#define SLIDWINDOW_MAXSIZE 8192

#define ACK_RECVSEQ_MODE 0x1
#define ACK_NORECVSEQ_MODE 0x2
#define ACK_RECVSEQ_BIT_MODE 0x3
#define ACK_NORECVSEQ_BIT_MODE 0x4

#define ACK_INIT_STATE 0x0      //0 为初始状态
#define ACK_RECV_STATE 0x1      //1 为已经收到回执
#define ACK_NORECV_STATE 0x2    //2 为请求重发（no ack）
//元信息
#define METTAINFO_SRCPORT 6000
#define METTAINFO_DSTPORT 6001
//元信息ack
#define METTAACK_SRCPORT 6002
#define METTAACK_DSTPORT 6003
//文件包
#define FILE_PKG_SRCPORT 6004
#define FILE_PKG_DSTPORT 6005
//文件包ack
#define FILE_PKG_ACK_SRCPORT 6006
#define FILE_PKG_ACK_DSTPORT 6007
//文件接收成功
#define FILE_SUC_SRCPORT 6008
#define FILE_SUC_DSTPORT 6009

//文件ack buf最大长度
#define ACKBUF_MAX 1000
#define ACKBUF_BIT_MAX 8000

//接收暂存的文件名
#define RECV_TEMP_FILENAME "/recv.temp"



struct PackageFrame
{
    unsigned int index;
    unsigned int recvAck;//0 为初始状态、1 为已经收到回执、2 为请求重发（no ack）、3已经收到
    QByteArray frameArray;

    PackageFrame()
    {
        index = 0;
        recvAck = 0;
    }
};

#endif // COMMONDEFINE_H
