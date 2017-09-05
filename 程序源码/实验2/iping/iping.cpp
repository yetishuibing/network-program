#include <iostream.h>
#include <iomanip.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include "iping.h"

int main(int argc,char *argv[])
{
	if (argc != 2)
	{
		cerr << "\nUsage: itracert ip_or_hostname\n";
		return -1;
	}

	//��ʼ��winsock2����
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		cerr << "\nFailed to initialize the WinSock2 DLL\n"
			 << "error code: " << WSAGetLastError() << endl;
		return -1;
	}

	//�������в���ת��ΪIP��ַ
	u_long ulDestIP = inet_addr(argv[1]);
	if (ulDestIP == INADDR_NONE)
	{
		//ת�����ɹ�ʱ����������
		hostent* pHostent = gethostbyname(argv[1]);
		if (pHostent)
		{
			ulDestIP = (*(in_addr*)pHostent->h_addr).s_addr;

			//�����Ļ��Ϣ
			cout << "\nPinging to " << argv[1] 
				 << " [" << inet_ntoa(*(in_addr*)(&ulDestIP)) << "]"
				 << " with " << DEF_ICMP_DATA_SIZE << " bytes of data:\n" << endl;
		}
		else //����������ʧ��
		{
			cerr << "\nCould not resolve the host name " << argv[1] << '\n'
				 << "error code: " << WSAGetLastError() << endl;
			WSACleanup();
			return -1;
		}
	}
	else
	{
		//�����Ļ��Ϣ
		cout << "\nPinging to " << argv[1] 
			 << " with " << DEF_ICMP_DATA_SIZE << " bytes of data:\n" << endl;
	}
	
	//���Ŀ��Socket��ַ
	sockaddr_in destSockAddr;
	ZeroMemory(&destSockAddr, sizeof(sockaddr_in));
	destSockAddr.sin_family = AF_INET;
	destSockAddr.sin_addr.s_addr = ulDestIP;
	
	//ʹ��ICMPЭ�鴴��Raw Socket
	SOCKET sockRaw = WSASocket(AF_INET, SOCK_RAW, IPPROTO_ICMP, NULL, 0, WSA_FLAG_OVERLAPPED);
	if (sockRaw == INVALID_SOCKET)
	{
		cerr << "\nFailed to create a raw socket\n"
			 << "error code: " << WSAGetLastError() << endl;
		WSACleanup();
		return -1;
	}
	//���ö˿�����
	int iTimeout = DEF_ICMP_TIMEOUT;

	if (setsockopt(sockRaw, SOL_SOCKET, SO_RCVTIMEO, (char*)&iTimeout, sizeof(iTimeout)) == SOCKET_ERROR)
	{
		cerr << "\nFailed to set recv timeout\n"
			 << "error code: " << WSAGetLastError() << endl;
		closesocket(sockRaw);
		WSACleanup();
		return -1;
	}
	if (setsockopt(sockRaw, SOL_SOCKET, SO_SNDTIMEO, (char*)&iTimeout, sizeof(iTimeout)) == SOCKET_ERROR)
	{
		cerr << "\nFailed to set send timeout\n"
			 << "error code: " << WSAGetLastError() << endl;
		closesocket(sockRaw);
		WSACleanup();
		return -1;
	}

	char IcmpSendBuf[sizeof(ICMP_HEADER)+DEF_ICMP_DATA_SIZE];
	memset(IcmpSendBuf,0,sizeof(IcmpSendBuf));
	char IcmpRecvBuf[MAX_ICMP_PACKET_SIZE];
	memset(IcmpRecvBuf,0,sizeof(IcmpRecvBuf));
	
	//���icmp���ݰ����ֶ�
	ICMP_HEADER *pIcmpHeader = (ICMP_HEADER *)IcmpSendBuf;
	pIcmpHeader->type = ICMP_ECHO_REQUEST;
	pIcmpHeader->code = 0;
	pIcmpHeader->id = (USHORT)GetCurrentProcessId();
	memset(IcmpSendBuf+sizeof(ICMP_HEADER),'E',DEF_ICMP_DATA_SIZE);
	
	//ѭ������3���������icmp���ݰ�
	DECODE_RESULT stDecodeResult;
	for(int usSeqNo = 0;usSeqNo <= 3;usSeqNo++)
	{
		//����seq��cksum����������ÿ��ѭ������Ҫ�ı�Ĳ��������Է���ѭ���ڲ�ÿ��ѭ���������޸�
		pIcmpHeader->seq = htons(usSeqNo);
		pIcmpHeader->cksum = 0;
		pIcmpHeader->cksum = GenerateChecksum((USHORT *)IcmpSendBuf,sizeof(ICMP_HEADER)+DEF_ICMP_DATA_SIZE);

		//��¼���кź͵�ǰʱ��
		stDecodeResult.usSeqNo = usSeqNo;
		stDecodeResult.dwRoundTripTime = GetTickCount();

		//����ICMP��EchoRequest���ݱ�
		if (sendto(sockRaw, IcmpSendBuf, sizeof(IcmpSendBuf), 0, 
				   (sockaddr*)&destSockAddr, sizeof(destSockAddr)) == SOCKET_ERROR)
		{
			//���Ŀ���������ɴ���ֱ���˳�
			if (WSAGetLastError() == WSAEHOSTUNREACH)
				cout << '\t' << "Destination host unreachable.\n" 
					 << "\nPinging complete.\n" << endl;
			closesocket(sockRaw);
			WSACleanup();
			return 0;
		}
		
		sockaddr_in from;
		int iFromLen = sizeof(from);
		int iReadLen;
		while(1)
		{
			iReadLen = recvfrom(sockRaw,IcmpRecvBuf,MAX_ICMP_PACKET_SIZE,0,(sockaddr *)&from,&iFromLen);
			if(iReadLen != SOCKET_ERROR)  //��ȷ���յ�һ��icmp���ݰ�
			{
				//������յ������ݰ����ж��Ƿ��Ƿ��ظ������̵�icmp��Ӧ���ݰ�
				if(DecodeIcmpResponse(IcmpRecvBuf,iReadLen,stDecodeResult) && (stDecodeResult.dwIPaddr.s_addr == destSockAddr.sin_addr.s_addr))
					cout << "Reply from " << inet_ntoa(stDecodeResult.dwIPaddr) <<" : bytes=" << DEF_ICMP_DATA_SIZE 
						 << " RTT=" << stDecodeResult.dwRoundTripTime << "ms, TTL=" << stDecodeResult.iTTL << "." << endl;
				break;
				
			}
			else if(WSAGetLastError() == WSAETIMEDOUT) //���ճ�ʱ
			{
				cout << "Request timed out." << endl;
				break;
			}
			else	//��������
			{
				cerr << "\nFailed to call recvfrom\n"
					 << "error code: " << WSAGetLastError() << endl;
				closesocket(sockRaw);
				WSACleanup();
				return -1;
			}
		}
	}

	//�����Ļ��Ϣ
	cout << "\nPing complete.\n" << endl;

	closesocket(sockRaw);
	WSACleanup();
	return 0;

}


//��������У���
USHORT GenerateChecksum(USHORT* pBuf, int iSize) 
{
	unsigned long cksum = 0;
	while (iSize>1) 
	{
		cksum += *pBuf++;
		iSize -= sizeof(USHORT);
	}
	if (iSize) 
		cksum += *(UCHAR*)pBuf;

	cksum = (cksum >> 16) + (cksum & 0xffff);
	cksum += (cksum >> 16);

	return (USHORT)(~cksum);
}

bool DecodeIcmpResponse(char* pBuf, int iPacketSize, DECODE_RESULT& stDecodeResult)
{	//��ȡ�յ�ip���ݰ����ײ���Ϣ
	IP_HEADER *pIpHrd = (IP_HEADER *)pBuf;
	int iIpHrdLen = pIpHrd->hdr_len * 4;
	if (iPacketSize < (int)(iIpHrdLen + sizeof(ICMP_HEADER)))
		return false;

	//ָ��ָ��icmp���ݰ����׵�ַ
	ICMP_HEADER *pIcmpHrd = (ICMP_HEADER *)(pBuf+iIpHrdLen);
	
	USHORT usID,usSquNo;
	//��õ����ݰ���type�ֶ�ΪICMP_ECHO_REPLY�����յ�һ������Ӧ��icmp���ݰ�
	if (pIcmpHrd->type == ICMP_ECHO_REPLY)  
	{
		usID = pIcmpHrd->id;
		usSquNo = ntohs(pIcmpHrd->seq);  //���յ����������ֽ�˳���seq�ֶ���Ϣ����Ҫת�������ֽ�˳��
	}

	if(usID != GetCurrentProcessId() || usSquNo != stDecodeResult.usSeqNo)
		return false;

	//��¼�Է�������IP��ַ�Լ�����������ʱRTT
	if (pIcmpHrd->type == ICMP_ECHO_REPLY)
	{
		stDecodeResult.iTTL = pIpHrd->ttl;
		stDecodeResult.dwIPaddr.s_addr = pIpHrd->sourceIP;
		stDecodeResult.dwRoundTripTime = GetTickCount() - stDecodeResult.dwRoundTripTime;
		return true;
	}
	return false;
}