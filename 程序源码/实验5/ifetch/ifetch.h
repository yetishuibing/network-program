#include <stdlib.h>
#include <stdio.h>
#include <winsock2.h> 
#include <Iphlpapi.h>		//非VC++6.0 标准SDK头文件，需要从外部加载 
#include <windows.h>
#include <string.h>
#pragma comment(lib,"ws2_32.lib")
#pragma comment(lib,"IPHLPAPI.LIB")		//非VC++6.0 标准SDK库文件，需要从外部加载 
#pragma pack(2)		//设置字节对齐属性，不设置会导致某些结构体的sizeof()偏大

#define PORT 53		//DNS协议的端口号
#define TIMEOUT 3000	//超时时间

typedef struct		//DNS报文首部
{
    unsigned short id;
    unsigned short flags;
    unsigned short questNum;
    unsigned short answerNum;
    unsigned short authorNum;
    unsigned short additionNum;
}DNSHDR,*pDNSHDR;	

typedef struct		//DNS报文查询记录
{
    unsigned short type;
    unsigned short queryclass;
}QUERYHDR,*pQUERYHDR;

typedef struct		//DNS报文应答记录
{
 unsigned short type;
 unsigned short classes;
 unsigned long  ttl;
 unsigned short length;
}RESPONSE,*pRESPONSE;


int genDNSPacket(pDNSHDR pDnsHdr,pQUERYHDR pQueryHdr,char *hostname,char *DNSsendBuff);
void decodeDNSPacket(char * DNSrecvBuff);
void getDnsServer(char *dnsServer);