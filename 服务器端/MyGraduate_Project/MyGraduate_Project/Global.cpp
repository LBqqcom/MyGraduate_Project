#include "Global.h"
#include "Logger.h"
#include <fcntl.h>
#include <cstdlib>
#include <string.h>
//���÷�����ģʽ
void global::setnonblocking(int sock)
{
	int opts;
	opts = fcntl(sock, F_GETFL);
	if (opts < 0)
	{
		LogError("fcntl(sock,GETFL) error:%s", strerror(errno));
		exit(1);
	}
	opts = opts | O_NONBLOCK;
	if (fcntl(sock, F_SETFL, opts) < 0)
	{
		LogError("fcntl(sock,SETFL,opts) error:%s", strerror(errno));
		exit(1);
	}
}
//��ӡ�ַ�����ǰlen���ֽ�
void global::PrintData(char * data, int len)
{
	char intent[1024] = { 0 };
	memcpy(intent, data, len);
	printf("data is:%s\n", intent);
}
