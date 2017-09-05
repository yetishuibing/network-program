#include <stdlib.h>
#include <stdio.h>
#include <winsock2.h> 
#include <Iphlpapi.h>		//��VC++6.0 ��׼SDKͷ�ļ�����Ҫ���ⲿ���� 
#include <windows.h>
#include <string.h>
#pragma comment(lib,"ws2_32.lib")
#pragma comment(lib,"IPHLPAPI.LIB")		//��VC++6.0 ��׼SDK���ļ�����Ҫ���ⲿ���� 
#pragma pack(2)		//�����ֽڶ������ԣ������ûᵼ��ĳЩ�ṹ���sizeof()ƫ��

#define PORT 53		//DNSЭ��Ķ˿ں�
#define TIMEOUT 3000	//��ʱʱ��

typedef struct		//DNS�����ײ�
{
    unsigned short id;
    unsigned short flags;
    unsigned short questNum;
    unsigned short answerNum;
    unsigned short authorNum;
    unsigned short additionNum;
}DNSHDR,*pDNSHDR;	

typedef struct		//DNS���Ĳ�ѯ��¼
{
    unsigned short type;
    unsigned short queryclass;
}QUERYHDR,*pQUERYHDR;

typedef struct		//DNS����Ӧ���¼
{
 unsigned short type;
 unsigned short classes;
 unsigned long  ttl;
 unsigned short length;
}RESPONSE,*pRESPONSE;


int genDNSPacket(pDNSHDR pDnsHdr,pQUERYHDR pQueryHdr,char *hostname,char *DNSsendBuff);
void decodeDNSPacket(char * DNSrecvBuff);
void getDnsServer(char *dnsServer);