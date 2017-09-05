#include "ifetch.h"

int main()
{
    char sendBuff[1024];		//发送缓冲区
    char recvBuff[1024*10];		//接收缓冲区
	char hostname[100];			//保存DNS报文中的“域名”字段，这个字段在报文中长度为可变的
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2,2),&wsaData);		
	SOCKET SocketFd;
    int iServerLen;
    struct sockaddr_in ServerAddr,HostAddr;
    ZeroMemory(&ServerAddr,sizeof(struct sockaddr_in));
    SocketFd = socket(AF_INET,SOCK_DGRAM,0);		//创建一个数据报(SOCK_DGRAM)套接字
	int iTimeout = TIMEOUT;
	if(setsockopt(SocketFd,SOL_SOCKET,SO_RCVTIMEO,(char *)&iTimeout,sizeof(iTimeout)) == SOCKET_ERROR)	//设置接收超时时间为3000ms
	{
		printf("setsockopt(SO_RCVTIMEO) error: %d",WSAGetLastError());
		return -1;
	}
	HostAddr.sin_family = AF_INET;
	HostAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	if(bind(SocketFd,(struct sockaddr *)&HostAddr,sizeof(sockaddr_in)) == SOCKET_ERROR)	//将创建的socket和本地网卡绑定在一起
	{
		printf("binding error: %d", WSAGetLastError());
		return -1;	
	}
    
	ServerAddr.sin_family = AF_INET;		//获得DNS服务器地址并保存在ServerAddr里面
	char dnsServer[16];		
	getDnsServer(dnsServer);		//获取本机DNS服务器地址，作为字符串形式保存在dnsServer里面，如字符串"202.103.24.68"
	printf("Default DNS Server: %s\n\n",dnsServer);
	unsigned long ulDestIP = inet_addr(dnsServer);	//将点分十进制计法的字符串IP地址转换为无符号32位长整形	
	ServerAddr.sin_addr.s_addr = ulDestIP;
    ServerAddr.sin_port = htons(PORT);
    iServerLen = sizeof(ServerAddr);
	pDNSHDR pDnsHdr = (pDNSHDR)malloc(sizeof(DNSHDR));
    pQUERYHDR pQueryHdr = (pQUERYHDR)malloc(sizeof(QUERYHDR));
    while(1)
	{
		printf(">");
		scanf("%s",hostname);
		int iSendByte = genDNSPacket(pDnsHdr,pQueryHdr,hostname,sendBuff);		//组装一个DNS请求报文，保存在sendBuff里面，并返回这个报文的长度保存在变量iSendBuff里面
		if (iSendByte == -1) 
		{
			return -1;
		}
		int iRes;
		iRes = sendto(SocketFd,sendBuff,iSendByte,0,(struct sockaddr*)&ServerAddr,iServerLen);	//将sendBuff中的DNS请求报文发往DNS服务器
		if(iRes == -1)
		{
			printf("sendto error: %d", WSAGetLastError());
			return -1;
		}
		iRes = recvfrom(SocketFd,recvBuff,sizeof(recvBuff),0,(struct sockaddr*)&ServerAddr,&iServerLen);		
		decodeDNSPacket(recvBuff);		//将DNS应答报文接收到recvBuff，并进行解析
		ZeroMemory(recvBuff,sizeof(recvBuff));		//清空接收缓存里面的数据
		ZeroMemory(hostname,sizeof(hostname));
	}
	free(pDnsHdr);			//释放之前malloc()申请的内存空间
	free(pQueryHdr);
	WSACleanup();
	return 0;    
}


//生成DNS查询报文，保存在DNSsendBuff里面，hostname为需要查询的域名字符串
int genDNSPacket(pDNSHDR pDnsHdr,pQUERYHDR pQueryHdr,char *hostname,char *DNSsendBuff)
{
    if(!(strcmp(hostname,"exit")))		//如果在命令行输入“exit”则函数结束返回-1，程序结束
	{
		return -1;
	}
	else		//正常的DNS查询请求
	{
		int iSendByte = 0;
		ZeroMemory(DNSsendBuff,sizeof(DNSsendBuff));
		pDnsHdr->id = htons(0x0000);		//“标识”字段设置为0
		pDnsHdr->flags = htons(0x0100);			//“标志”字段设置为0x0100，即RD位为1，期望递归查询
		pDnsHdr->questNum = htons(0x0001);		//1个查询记录
		pDnsHdr->answerNum = htons(0x0000);		//没有回答记录和其他的记录
		pDnsHdr->authorNum = htons(0x0000);
		pDnsHdr->additionNum = htons(0x0000);
		memcpy(DNSsendBuff,pDnsHdr,sizeof(DNSHDR));		//把生成的DNS查询报文首部复制到DNSsendBuff里面
		iSendByte += sizeof(DNSHDR);		//记录目前DNSsendBuff里面的数据量
	
		//对域名字符串进行解析并进行形式的变换，例如将"www.wust.edu.cn\0"变成"3www4wust3edu2cn0x00"
		char *pTrace = hostname;		
		char *pHostname = hostname;
		int iStrLen = strlen(hostname);
		unsigned char iCharNum = 0;
		while(*pTrace != '\0')		//指针移到最后并从最后一个字符'\0'开始，每个字符往后移一个字节
		{
			pTrace++;
		}
		while(pTrace != hostname)
		{
			*(pTrace+1) = *pTrace;
			pTrace--;
		}
		*(pTrace+1) = *pTrace;		//把第一个字符移到第二个字符位置
		pTrace++;		//此时第一个字符没有实际意义，将指针指向原字符串中第二个字符的位置
		while(*pTrace != '\0')		//从第一个字符开始扫描，iCharNum统计每两个字符'.'之间的字符数，然后填入原来字符'.'的位置
		{
			if(*pTrace == '.')
			{
				*pHostname = iCharNum;
				iCharNum = 0;
				pHostname = pTrace;
			}
			else
			{
				iCharNum ++;
			}
			pTrace ++;
		}
		*pHostname = iCharNum;		//最后一个字符'.'之后的字符数写入，例如"3www6google3com.hk"中的".hk"

		memcpy(DNSsendBuff + sizeof(DNSHDR),hostname,iStrLen+2);	//将解析好的“域名”字段填入相应的位置
		iSendByte += (iStrLen + 2);									//之所以为iStrLen+2，是因为解析后的结果比原来多了2个字符
																	//即最开始的位置需要填入一个数字和最后需要填入0x00。
		pQueryHdr->type = htons(0x0001);
		pQueryHdr->queryclass = htons(0x0001);

		memcpy(DNSsendBuff + sizeof(DNSHDR) + iStrLen + 2,pQueryHdr,sizeof(QUERYHDR));	//在“域名”字段之后填入“查询类型”和“查询类”的值
		iSendByte += sizeof(QUERYHDR);		//累加生成的DNSsendBuff里面的数据的字节数
		return iSendByte;			//返回最终生成的DNS查询报文的字节数
	}	
}

//解析收到的DNS应答报文DNSrecvBuff，应答记录字段中类型值为5即为别名则直接移动指针跳过此记录，类型值为1即为查询域名对应的IP地址信息就显示出来
void decodeDNSPacket(char *DNSrecvBuff)
{
	pDNSHDR pDnsHdr = (pDNSHDR)DNSrecvBuff;			//指针pDnsHdr指向接收到的DNS应答报文首部
	int iQueryNum,iRespNum,iAuthRespNum,iAddtionNum;
	iQueryNum = ntohs(pDnsHdr->questNum);			//将查询记录数目
	iRespNum = ntohs(pDnsHdr->answerNum);			//	应答记录数目
	iAuthRespNum = ntohs(pDnsHdr->authorNum);		//	授权服务器地址数目
	iAddtionNum = ntohs(pDnsHdr->additionNum);		//	附加信息数目均保存起来，本程序主要使用查询记录数目和应答记录数目
	
	
	if(pDnsHdr->flags >> 15)	//将DNS应答报文的“标志”字段右移15位即取最高位，为0为DNS查询报文，为1则为DNS应答报文
	{
		
		if((pDnsHdr->flags & 0x0007) == 3)
		{
			printf("No corresponding domain name entry.\n\n");
			return;
		}
		if((pDnsHdr->flags >> 10) & 0x0001)	//查看“标志”字段的AA位，看此应答信息是否为授权应答
		{
			printf("Authoritative answer:\n");
		}
		else
		{
			printf("None-authoritative answer:\n");
		}

		char *pTraceResponse;
		pTraceResponse=DNSrecvBuff+sizeof(DNSHDR);	//指针移向应答报文中的第一个查询记录，因为一般情况下应答报文均会首先附带一个对应的查询记录
		while(*pTraceResponse)		//因为查询记录的“域名”字段是变长的，以0x00结尾，所以通过移动指针将指针移到“域名”字段最后
			pTraceResponse++;	
		pTraceResponse++;			//把指针移动到查询记录的“域名”字段之后
		pTraceResponse += sizeof(long);		//跳过“查询类型”和“查询类”两个字段，指针指向第一个应答记录
		in_addr address;
		pRESPONSE pResponse;
		printf("Addresses: ");
		for(int i=1;i<=iRespNum;i++)
		{

			pTraceResponse += sizeof(short);  //指针跳过应答记录的“域名”字段，此“域名”字段一般为一个域名指针，以0xC0开始。
			pResponse = (pRESPONSE)pTraceResponse;
			if(ntohs(pResponse->type) == 1)		//这条应答记录返回的是与之前查询所对应的IP地址
			{
				pTraceResponse += sizeof(RESPONSE);
				unsigned long ulIP = *(unsigned long *)pTraceResponse;	
				address.s_addr = ulIP;		//获取IP地址信息保存到ulIP，并写入address里面
				if(i == iRespNum)	//最后一条记录显示句号，否则显示分号
				{
					printf("%s. ",inet_ntoa(address));
				}
				else
				{
					printf("%s; ",inet_ntoa(address));
				}
				pTraceResponse += sizeof(long);		//指针移过应答记录的IP地址字段，指向下一个应答记录	
			}
			else if (ntohs(pResponse->type) == 5)		//这条应答记录为所查询主机的一个别名，这里本程序直接跳过这条记录
			{
				pTraceResponse += sizeof(RESPONSE);
				pTraceResponse += ntohs(pResponse->length);
			}
		}
		printf("\n\n");
	}
	else		//“标志”字段最高位不为1，表示不是一个DNS应答报文，不做任何处理
	{
		printf("Invalid DNS resolution!\n\n");
	}
}

//此函数获取本机DNS服务器地址（为点分十进制计法的字符串形式），并保存到dnsServer中，这里需要Iphlpapi.h和IPHLPAPI.LIB
void getDnsServer(char *dnsServer)
{
	//获得需要的缓冲区大小
	DWORD nLength = 0;
	if (GetNetworkParams(NULL, &nLength) != ERROR_BUFFER_OVERFLOW)
	{
		return;
	}

	FIXED_INFO* pFixedInfo = (FIXED_INFO*)new BYTE[nLength];

	//获得本地计算机网络参数
	if (GetNetworkParams(pFixedInfo, &nLength) != ERROR_SUCCESS)
	{
		delete[] pFixedInfo;
		return;
	}
	IP_ADDR_STRING* pCurrentDnsServer = &pFixedInfo->DnsServerList;
	if(pCurrentDnsServer != NULL)
	{
		char *tmp = pCurrentDnsServer->IpAddress.String;	//pCurrentDnsServer->IpAddress.String即为我们所需要的字符串形式的DNS服务器IP地址
		strcpy(dnsServer,tmp);
	}	
}

