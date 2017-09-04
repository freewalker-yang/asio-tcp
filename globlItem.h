#ifndef GLOBLA_ITEM_H
#define GLOBAL_ITEM_H

typedef unsigned int UINT;
typedef unsigned char BYTE;


//��ͷ
typedef struct _header
{
	UINT		len;	//���ݳ���
	BYTE		type;	//������
	BYTE		subtype;
	UINT        packNum; //�����
}HDR, *PHDER;

const int HEADER_SIZE = sizeof(HDR);

#endif //