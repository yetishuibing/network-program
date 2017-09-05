#include "ifetch.h"

int main()
{
    char sendBuff[1024];		//���ͻ�����
    char recvBuff[1024*10];		//���ջ�����
	char hostname[100];			//����DNS�����еġ��������ֶΣ�����ֶ��ڱ����г���Ϊ�ɱ��
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2,2),&wsaData);		
	SOCKET SocketFd;
    int iServerLen;
    struct sockaddr_in ServerAddr,HostAddr;
    ZeroMemory(&ServerAddr,sizeof(struct sockaddr_in));
    SocketFd = socket(AF_INET,SOCK_DGRAM,0);		//����һ�����ݱ�(SOCK_DGRAM)�׽���
	int iTimeout = TIMEOUT;
	if(setsockopt(SocketFd,SOL_SOCKET,SO_RCVTIMEO,(char *)&iTimeout,sizeof(iTimeout)) == SOCKET_ERROR)	//���ý��ճ�ʱʱ��Ϊ3000ms
	{
		printf("setsockopt(SO_RCVTIMEO) error: %d",WSAGetLastError());
		return -1;
	}
	HostAddr.sin_family = AF_INET;
	HostAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	if(bind(SocketFd,(struct sockaddr *)&HostAddr,sizeof(sockaddr_in)) == SOCKET_ERROR)	//��������socket�ͱ�����������һ��
	{
		printf("binding error: %d", WSAGetLastError());
		return -1;	
	}
    
	ServerAddr.sin_family = AF_INET;		//���DNS��������ַ��������ServerAddr����
	char dnsServer[16];		
	getDnsServer(dnsServer);		//��ȡ����DNS��������ַ����Ϊ�ַ�����ʽ������dnsServer���棬���ַ���"202.103.24.68"
	printf("Default DNS Server: %s\n\n",dnsServer);
	unsigned long ulDestIP = inet_addr(dnsServer);	//�����ʮ���ƼƷ����ַ���IP��ַת��Ϊ�޷���32λ������	
	ServerAddr.sin_addr.s_addr = ulDestIP;
    ServerAddr.sin_port = htons(PORT);
    iServerLen = sizeof(ServerAddr);
	pDNSHDR pDnsHdr = (pDNSHDR)malloc(sizeof(DNSHDR));
    pQUERYHDR pQueryHdr = (pQUERYHDR)malloc(sizeof(QUERYHDR));
    while(1)
	{
		printf(">");
		scanf("%s",hostname);
		int iSendByte = genDNSPacket(pDnsHdr,pQueryHdr,hostname,sendBuff);		//��װһ��DNS�����ģ�������sendBuff���棬������������ĵĳ��ȱ����ڱ���iSendBuff����
		if (iSendByte == -1) 
		{
			return -1;
		}
		int iRes;
		iRes = sendto(SocketFd,sendBuff,iSendByte,0,(struct sockaddr*)&ServerAddr,iServerLen);	//��sendBuff�е�DNS�����ķ���DNS������
		if(iRes == -1)
		{
			printf("sendto error: %d", WSAGetLastError());
			return -1;
		}
		iRes = recvfrom(SocketFd,recvBuff,sizeof(recvBuff),0,(struct sockaddr*)&ServerAddr,&iServerLen);		
		decodeDNSPacket(recvBuff);		//��DNSӦ���Ľ��յ�recvBuff�������н���
		ZeroMemory(recvBuff,sizeof(recvBuff));		//��ս��ջ������������
		ZeroMemory(hostname,sizeof(hostname));
	}
	free(pDnsHdr);			//�ͷ�֮ǰmalloc()������ڴ�ռ�
	free(pQueryHdr);
	WSACleanup();
	return 0;    
}


//����DNS��ѯ���ģ�������DNSsendBuff���棬hostnameΪ��Ҫ��ѯ�������ַ���
int genDNSPacket(pDNSHDR pDnsHdr,pQUERYHDR pQueryHdr,char *hostname,char *DNSsendBuff)
{
    if(!(strcmp(hostname,"exit")))		//��������������롰exit��������������-1���������
	{
		return -1;
	}
	else		//������DNS��ѯ����
	{
		int iSendByte = 0;
		ZeroMemory(DNSsendBuff,sizeof(DNSsendBuff));
		pDnsHdr->id = htons(0x0000);		//����ʶ���ֶ�����Ϊ0
		pDnsHdr->flags = htons(0x0100);			//����־���ֶ�����Ϊ0x0100����RDλΪ1�������ݹ��ѯ
		pDnsHdr->questNum = htons(0x0001);		//1����ѯ��¼
		pDnsHdr->answerNum = htons(0x0000);		//û�лش��¼�������ļ�¼
		pDnsHdr->authorNum = htons(0x0000);
		pDnsHdr->additionNum = htons(0x0000);
		memcpy(DNSsendBuff,pDnsHdr,sizeof(DNSHDR));		//�����ɵ�DNS��ѯ�����ײ����Ƶ�DNSsendBuff����
		iSendByte += sizeof(DNSHDR);		//��¼ĿǰDNSsendBuff�����������
	
		//�������ַ������н�����������ʽ�ı任�����罫"www.wust.edu.cn\0"���"3www4wust3edu2cn0x00"
		char *pTrace = hostname;		
		char *pHostname = hostname;
		int iStrLen = strlen(hostname);
		unsigned char iCharNum = 0;
		while(*pTrace != '\0')		//ָ���Ƶ���󲢴����һ���ַ�'\0'��ʼ��ÿ���ַ�������һ���ֽ�
		{
			pTrace++;
		}
		while(pTrace != hostname)
		{
			*(pTrace+1) = *pTrace;
			pTrace--;
		}
		*(pTrace+1) = *pTrace;		//�ѵ�һ���ַ��Ƶ��ڶ����ַ�λ��
		pTrace++;		//��ʱ��һ���ַ�û��ʵ�����壬��ָ��ָ��ԭ�ַ����еڶ����ַ���λ��
		while(*pTrace != '\0')		//�ӵ�һ���ַ���ʼɨ�裬iCharNumͳ��ÿ�����ַ�'.'֮����ַ�����Ȼ������ԭ���ַ�'.'��λ��
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
		*pHostname = iCharNum;		//���һ���ַ�'.'֮����ַ���д�룬����"3www6google3com.hk"�е�".hk"

		memcpy(DNSsendBuff + sizeof(DNSHDR),hostname,iStrLen+2);	//�������õġ��������ֶ�������Ӧ��λ��
		iSendByte += (iStrLen + 2);									//֮����ΪiStrLen+2������Ϊ������Ľ����ԭ������2���ַ�
																	//���ʼ��λ����Ҫ����һ�����ֺ������Ҫ����0x00��
		pQueryHdr->type = htons(0x0001);
		pQueryHdr->queryclass = htons(0x0001);

		memcpy(DNSsendBuff + sizeof(DNSHDR) + iStrLen + 2,pQueryHdr,sizeof(QUERYHDR));	//�ڡ��������ֶ�֮�����롰��ѯ���͡��͡���ѯ�ࡱ��ֵ
		iSendByte += sizeof(QUERYHDR);		//�ۼ����ɵ�DNSsendBuff��������ݵ��ֽ���
		return iSendByte;			//�����������ɵ�DNS��ѯ���ĵ��ֽ���
	}	
}

//�����յ���DNSӦ����DNSrecvBuff��Ӧ���¼�ֶ�������ֵΪ5��Ϊ������ֱ���ƶ�ָ�������˼�¼������ֵΪ1��Ϊ��ѯ������Ӧ��IP��ַ��Ϣ����ʾ����
void decodeDNSPacket(char *DNSrecvBuff)
{
	pDNSHDR pDnsHdr = (pDNSHDR)DNSrecvBuff;			//ָ��pDnsHdrָ����յ���DNSӦ�����ײ�
	int iQueryNum,iRespNum,iAuthRespNum,iAddtionNum;
	iQueryNum = ntohs(pDnsHdr->questNum);			//����ѯ��¼��Ŀ
	iRespNum = ntohs(pDnsHdr->answerNum);			//	Ӧ���¼��Ŀ
	iAuthRespNum = ntohs(pDnsHdr->authorNum);		//	��Ȩ��������ַ��Ŀ
	iAddtionNum = ntohs(pDnsHdr->additionNum);		//	������Ϣ��Ŀ��������������������Ҫʹ�ò�ѯ��¼��Ŀ��Ӧ���¼��Ŀ
	
	
	if(pDnsHdr->flags >> 15)	//��DNSӦ���ĵġ���־���ֶ�����15λ��ȡ���λ��Ϊ0ΪDNS��ѯ���ģ�Ϊ1��ΪDNSӦ����
	{
		
		if((pDnsHdr->flags & 0x0007) == 3)
		{
			printf("No corresponding domain name entry.\n\n");
			return;
		}
		if((pDnsHdr->flags >> 10) & 0x0001)	//�鿴����־���ֶε�AAλ������Ӧ����Ϣ�Ƿ�Ϊ��ȨӦ��
		{
			printf("Authoritative answer:\n");
		}
		else
		{
			printf("None-authoritative answer:\n");
		}

		char *pTraceResponse;
		pTraceResponse=DNSrecvBuff+sizeof(DNSHDR);	//ָ������Ӧ�����еĵ�һ����ѯ��¼����Ϊһ�������Ӧ���ľ������ȸ���һ����Ӧ�Ĳ�ѯ��¼
		while(*pTraceResponse)		//��Ϊ��ѯ��¼�ġ��������ֶ��Ǳ䳤�ģ���0x00��β������ͨ���ƶ�ָ�뽫ָ���Ƶ����������ֶ����
			pTraceResponse++;	
		pTraceResponse++;			//��ָ���ƶ�����ѯ��¼�ġ��������ֶ�֮��
		pTraceResponse += sizeof(long);		//��������ѯ���͡��͡���ѯ�ࡱ�����ֶΣ�ָ��ָ���һ��Ӧ���¼
		in_addr address;
		pRESPONSE pResponse;
		printf("Addresses: ");
		for(int i=1;i<=iRespNum;i++)
		{

			pTraceResponse += sizeof(short);  //ָ������Ӧ���¼�ġ��������ֶΣ��ˡ��������ֶ�һ��Ϊһ������ָ�룬��0xC0��ʼ��
			pResponse = (pRESPONSE)pTraceResponse;
			if(ntohs(pResponse->type) == 1)		//����Ӧ���¼���ص�����֮ǰ��ѯ����Ӧ��IP��ַ
			{
				pTraceResponse += sizeof(RESPONSE);
				unsigned long ulIP = *(unsigned long *)pTraceResponse;	
				address.s_addr = ulIP;		//��ȡIP��ַ��Ϣ���浽ulIP����д��address����
				if(i == iRespNum)	//���һ����¼��ʾ��ţ�������ʾ�ֺ�
				{
					printf("%s. ",inet_ntoa(address));
				}
				else
				{
					printf("%s; ",inet_ntoa(address));
				}
				pTraceResponse += sizeof(long);		//ָ���ƹ�Ӧ���¼��IP��ַ�ֶΣ�ָ����һ��Ӧ���¼	
			}
			else if (ntohs(pResponse->type) == 5)		//����Ӧ���¼Ϊ����ѯ������һ�����������ﱾ����ֱ������������¼
			{
				pTraceResponse += sizeof(RESPONSE);
				pTraceResponse += ntohs(pResponse->length);
			}
		}
		printf("\n\n");
	}
	else		//����־���ֶ����λ��Ϊ1����ʾ����һ��DNSӦ���ģ������κδ���
	{
		printf("Invalid DNS resolution!\n\n");
	}
}

//�˺�����ȡ����DNS��������ַ��Ϊ���ʮ���ƼƷ����ַ�����ʽ���������浽dnsServer�У�������ҪIphlpapi.h��IPHLPAPI.LIB
void getDnsServer(char *dnsServer)
{
	//�����Ҫ�Ļ�������С
	DWORD nLength = 0;
	if (GetNetworkParams(NULL, &nLength) != ERROR_BUFFER_OVERFLOW)
	{
		return;
	}

	FIXED_INFO* pFixedInfo = (FIXED_INFO*)new BYTE[nLength];

	//��ñ��ؼ�����������
	if (GetNetworkParams(pFixedInfo, &nLength) != ERROR_SUCCESS)
	{
		delete[] pFixedInfo;
		return;
	}
	IP_ADDR_STRING* pCurrentDnsServer = &pFixedInfo->DnsServerList;
	if(pCurrentDnsServer != NULL)
	{
		char *tmp = pCurrentDnsServer->IpAddress.String;	//pCurrentDnsServer->IpAddress.String��Ϊ��������Ҫ���ַ�����ʽ��DNS������IP��ַ
		strcpy(dnsServer,tmp);
	}	
}

