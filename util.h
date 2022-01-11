#ifndef UTIL_H
#define UTIL_H


class Util
{
public:
    Util();

    //int转 大端字节
    void intToBigByteArray(int val,char* data);
    //short转 大端字节
    void shortToBigByteArray(short val,char* data);
    //大端字节转int
    void BytesToInt(char* data,int* val);
    //大端字节转short
    void BytesToShort( char* data,unsigned short* val);
    //计算crc16
    unsigned short crc16(const char* data,int len);


};

#endif // UTIL_H
