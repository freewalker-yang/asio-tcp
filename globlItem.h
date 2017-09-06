#ifndef GLOBLA_ITEM_H
#define GLOBAL_ITEM_H

typedef unsigned int UINT;
typedef unsigned char BYTE;


//包头
typedef struct _header
{
	size_t		len;	//数据长度
	BYTE		type;	//包类型
	BYTE		subtype;
	size_t      packNum; //包序号
}HDR, *PHDER;

const int HEADER_SIZE = sizeof(HDR);

#endif //