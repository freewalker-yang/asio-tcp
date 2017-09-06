#ifndef GLOBLA_ITEM_H
#define GLOBAL_ITEM_H

typedef unsigned int UINT;
typedef unsigned char BYTE;


//��ͷ
typedef struct _header
{
	size_t		len;	//���ݳ���
	BYTE		type;	//������
	BYTE		subtype;
	size_t      packNum; //�����
}HDR, *PHDER;

const int HEADER_SIZE = sizeof(HDR);

#endif //