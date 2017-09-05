#include"stdio.h"
#include"string.h"
#include"Winsock2.h"

int main()
{
	WSADATA wsaData;
	if(WSAStartup(MAKEWORD(2,2),&wsaData)==SOCKET_ERROR)
	{
		exit(0);
	}
	int nLen=256;
	char hostname[20];
	gethostname(hostname,nLen);
	//hostent *pHost = gethostbyname(hostname);
	//LPSTR lpAddr = pHost->h_addr_list[0];



	//printf("********************************\n\n");
	printf("********************************\n");
	printf("打印所有IP,输出如下：\n");
	char szHost[256] = {0};
	hostent *lpHost = gethostbyname(szHost);
	memset(szHost,0,sizeof(szHost));//内存初始化，将szHost当前位置后面的sizeof(szHost)个字节用0代替


	// 打印出所有IP地址
	in_addr addr;
	for(int i = 0; ; i++)
	{
		char *p = lpHost->h_addr_list[i];
		if(p == NULL)
			break;
		memcpy(&addr.S_un.S_addr, p, lpHost->h_length);//从p所指的内存地址开始拷贝lpHost->h_length个字节到&addr.S_un.S_addr所指的内存区域
		char *szIp = inet_ntoa(addr);//将32位的二进制数转化为字符串;
		printf("本机IP地址：%s \n", szIp);
	}
	printf("********************************\n");
	::WSACleanup();
}
/*
#include"stdio.h"
#include"string.h"
#include"Winsock2.h"
//#pragma comment(lib,"WS2_32.lib")

int main()
{	printf("********************************\n");
	printf("方法一，精确输出\n");
	WSADATA wsaData;
	if(WSAStartup(MAKEWORD(2,2),&wsaData)==SOCKET_ERROR)
	{
		exit(0);
	}
	int nLen=256;
	char hostname[20];
	gethostname(hostname,nLen);
	hostent *pHost = gethostbyname(hostname);
	LPSTR lpAddr = pHost->h_addr_list[0];
	struct in_addr inAddr;
	memmove(&inAddr,lpAddr,4);//从lpAddr拷贝4个字节到inAddr
	printf("有  线  IP地址：%s\n",inet_ntoa(inAddr));//将网络地址转换成“.”间隔的字符串
	memmove(&inAddr,lpAddr+4,4);
	printf("无  线  IP地址：%s\n",inet_ntoa(inAddr));
	memmove(&inAddr,lpAddr+8,4);
	printf("环  回  IP地址：%s\n",inet_ntoa(inAddr));
	memmove(&inAddr,lpAddr+12,4);
	printf("虚拟机1 IP地址：%s\n",inet_ntoa(inAddr));
	memmove(&inAddr,lpAddr+16,4);
	printf("虚拟机2 IP地址：%s\n",inet_ntoa(inAddr));


	printf("********************************\n\n");
	printf("********************************\n");
	printf("方法二,打印所有Ip,验证第一次输出\n");
	char szHost[256] = {0};
	hostent *lpHost = gethostbyname(szHost);
	memset(szHost,0,sizeof(szHost));//内存初始化，将szHost当前位置后面的sizeof(szHost)个字节用0代替


	// 打印出所有IP地址
	in_addr addr;
	for(int i = 0; ; i++)
	{
		char *p = lpHost->h_addr_list[i];
		if(p == NULL)
			break;
		memcpy(&addr.S_un.S_addr, p, lpHost->h_length);//从p所指的内存地址开始拷贝lpHost->h_length个字节到&addr.S_un.S_addr所指的内存区域
		char *szIp = inet_ntoa(addr);//将32位的二进制数转化为字符串;
		printf("本机IP地址：%s \n", szIp);
	}
	printf("********************************\n");
	//::WSACleanup();
}

*/
