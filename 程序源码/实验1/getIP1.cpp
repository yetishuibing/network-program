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
	printf("��ӡ����IP,������£�\n");
	char szHost[256] = {0};
	hostent *lpHost = gethostbyname(szHost);
	memset(szHost,0,sizeof(szHost));//�ڴ��ʼ������szHost��ǰλ�ú����sizeof(szHost)���ֽ���0����


	// ��ӡ������IP��ַ
	in_addr addr;
	for(int i = 0; ; i++)
	{
		char *p = lpHost->h_addr_list[i];
		if(p == NULL)
			break;
		memcpy(&addr.S_un.S_addr, p, lpHost->h_length);//��p��ָ���ڴ��ַ��ʼ����lpHost->h_length���ֽڵ�&addr.S_un.S_addr��ָ���ڴ�����
		char *szIp = inet_ntoa(addr);//��32λ�Ķ�������ת��Ϊ�ַ���;
		printf("����IP��ַ��%s \n", szIp);
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
	printf("����һ����ȷ���\n");
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
	memmove(&inAddr,lpAddr,4);//��lpAddr����4���ֽڵ�inAddr
	printf("��  ��  IP��ַ��%s\n",inet_ntoa(inAddr));//�������ַת���ɡ�.��������ַ���
	memmove(&inAddr,lpAddr+4,4);
	printf("��  ��  IP��ַ��%s\n",inet_ntoa(inAddr));
	memmove(&inAddr,lpAddr+8,4);
	printf("��  ��  IP��ַ��%s\n",inet_ntoa(inAddr));
	memmove(&inAddr,lpAddr+12,4);
	printf("�����1 IP��ַ��%s\n",inet_ntoa(inAddr));
	memmove(&inAddr,lpAddr+16,4);
	printf("�����2 IP��ַ��%s\n",inet_ntoa(inAddr));


	printf("********************************\n\n");
	printf("********************************\n");
	printf("������,��ӡ����Ip,��֤��һ�����\n");
	char szHost[256] = {0};
	hostent *lpHost = gethostbyname(szHost);
	memset(szHost,0,sizeof(szHost));//�ڴ��ʼ������szHost��ǰλ�ú����sizeof(szHost)���ֽ���0����


	// ��ӡ������IP��ַ
	in_addr addr;
	for(int i = 0; ; i++)
	{
		char *p = lpHost->h_addr_list[i];
		if(p == NULL)
			break;
		memcpy(&addr.S_un.S_addr, p, lpHost->h_length);//��p��ָ���ڴ��ַ��ʼ����lpHost->h_length���ֽڵ�&addr.S_un.S_addr��ָ���ڴ�����
		char *szIp = inet_ntoa(addr);//��32λ�Ķ�������ת��Ϊ�ַ���;
		printf("����IP��ַ��%s \n", szIp);
	}
	printf("********************************\n");
	//::WSACleanup();
}

*/
