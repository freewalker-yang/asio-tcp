#ifndef GLOBLA_ITEM_H
#define GLOBAL_ITEM_H


#define MSG_BUFFER_ALLOCATOR  1 //enable the msg buffer allocator or not
#define IO_THREAD_IN_CLASS    1 //enable class internal io thread

#include <string>

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

void std_string_format(std::string & _str, const char * _Format, ...);

#endif //