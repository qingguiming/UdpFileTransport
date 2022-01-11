#include "util.h"
#include <string.h>
Util::Util()
{

}

void Util::intToBigByteArray(int val, char *data)
{

    data[0] = (val >> 24) & 0xFF;
    data[1] = (val >> 16) & 0xFF;
    data[2] = (val >> 8) & 0xFF;
    data[3] = val & 0xFF;

    //memcpy(data,&val,sizeof(val));

}

void Util::shortToBigByteArray(short val, char *data)
{
    data[0] = val >> 8;
    data[1] = val & 0xFF;


    //memcpy(data,&val,sizeof(val));

}

void Util::BytesToInt(char *data, int *val)
{
    char temp[4] = {0};
    temp[0] = data[3];
    temp[1] = data[2];
    temp[2] = data[1];
    temp[3] = data[0];

    memcpy(val,&temp,sizeof(int));

}

void Util::BytesToShort(char *data, unsigned short *val)
{
    char temp[2] = {0};
    temp[0] = data[1];
    temp[1] = data[0];
    memcpy(val,&temp,sizeof(short));

}

unsigned short Util::crc16(const char *data, int len)
{
    return 0;
}
