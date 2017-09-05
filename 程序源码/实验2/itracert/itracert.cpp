/*----------------------------------------------------------
����˵�����ó����ʵ����Windows����ϵͳ��tracert����ܣ�
      �������IP���Ĵӱ�����������Ŀ��������������·����Ϣ��
ע�⣺�������ʱӦʹ��1�ֽڶ��뷽ʽ�����߽�!
-----------------------------------------------------------*/
#include <iostream.h>
#include <iomanip.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include "itracert.h"

////////////////////////////////////////////////////////

int main(int argc, char* argv[])
{
	//��������в���
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
			cout << "\nTracing route to " << argv[1] 
				 << " [" << inet_ntoa(*(in_addr*)(&ulDestIP)) << "]"
				 << " with a maximum of " << DEF_MAX_HOP << " hops.\n" << endl;
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
		cout << "\nTracing route to " << argv[1] 
			 << " with a maximum of " << DEF_MAX_HOP << " hops.\n" << endl;
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


	//����ICMP�����ͻ������ͽ��ջ�����
	char IcmpSendBuf[sizeof(ICMP_HEADER)+DEF_ICMP_DATA_SIZE];
	memset(IcmpSendBuf, 0, sizeof(IcmpSendBuf));
	char IcmpRecvBuf[MAX_ICMP_PACKET_SIZE];
	memset(IcmpRecvBuf, 0, sizeof(IcmpRecvBuf));

	//�������͵�ICMP��
	ICMP_HEADER* pIcmpHeader = (ICMP_HEADER*)IcmpSendBuf;
	pIcmpHeader->type = ICMP_ECHO_REQUEST;
	pIcmpHeader->code = 0;
	pIcmpHeader->id = (USHORT)GetCurrentProcessId();
	memset(IcmpSendBuf+sizeof(ICMP_HEADER), 'E', DEF_ICMP_DATA_SIZE);

	//��ʼ̽��·��
	DECODE_RESULT stDecodeResult;
	BOOL bReachDestHost = FALSE;
	USHORT usSeqNo = 0;
	int iTTL = 1;
	int iMaxHop = DEF_MAX_HOP;
	while (!bReachDestHost && iMaxHop--)
	{
		//����IP���ݱ�ͷ��ttl�ֶ�
		setsockopt(sockRaw, IPPROTO_IP, IP_TTL, (char*)&iTTL, sizeof(iTTL));

		//�����ǰ��վ����Ϊ·����Ϣ���
		cout << setw(3) << iTTL << flush;

		//���ICMP���ݱ�ʣ���ֶ�
		((ICMP_HEADER*)IcmpSendBuf)->cksum = 0;
		((ICMP_HEADER*)IcmpSendBuf)->seq = htons(usSeqNo++);
		((ICMP_HEADER*)IcmpSendBuf)->cksum = GenerateChecksum((USHORT*)IcmpSendBuf, sizeof(ICMP_HEADER)+DEF_ICMP_DATA_SIZE);
		
		//��¼���кź͵�ǰʱ��
		stDecodeResult.usSeqNo = ((ICMP_HEADER*)IcmpSendBuf)->seq;
		stDecodeResult.dwRoundTripTime = GetTickCount();
		
		//����ICMP��EchoRequest���ݱ�
		if (sendto(sockRaw, IcmpSendBuf, sizeof(IcmpSendBuf), 0, 
				   (sockaddr*)&destSockAddr, sizeof(destSockAddr)) == SOCKET_ERROR)
		{
			//���Ŀ���������ɴ���ֱ���˳�
			if (WSAGetLastError() == WSAEHOSTUNREACH)
				cout << '\t' << "Destination host unreachable.\n" 
					 << "\nTrace complete.\n" << endl;
			closesocket(sockRaw);
			WSACleanup();
			return 0;
		}

		//����ICMP��EchoReply���ݱ�
		//��Ϊ�յ��Ŀ��ܲ��ǳ������ڴ������ݱ���������Ҫѭ������ֱ���յ���Ҫ���ݻ�ʱ
		sockaddr_in from;
		int iFromLen = sizeof(from);
		int iReadDataLen;
		while (1)
		{
			//�ȴ����ݵ���
			iReadDataLen = recvfrom(sockRaw, IcmpRecvBuf, MAX_ICMP_PACKET_SIZE, 
									0, (sockaddr*)&from, &iFromLen);
			if (iReadDataLen != SOCKET_ERROR) //�����ݰ�����
			{
				//����õ������ݰ������������ȷ����������ѭ��������һ��EchoRequest��
				if (DecodeIcmpResponse(IcmpRecvBuf, iReadDataLen, stDecodeResult))
				{
					if (stDecodeResult.dwIPaddr.s_addr == destSockAddr.sin_addr.s_addr)
						bReachDestHost = TRUE;

					cout << '\t' << inet_ntoa(stDecodeResult.dwIPaddr) << endl;
					break;
				}
			}
			else if (WSAGetLastError() == WSAETIMEDOUT) //���ճ�ʱ����ӡ�Ǻ�
			{
				cout << setw(9) << '*' << '\t' << "Request timed out." << endl;
				break;
			}
			else
			{
				cerr << "\nFailed to call recvfrom\n"
					 << "error code: " << WSAGetLastError() << endl;
				closesocket(sockRaw);
				WSACleanup();
				return -1;
			}
		}

		//TTLֵ��1
		iTTL++;
	}
	//�����Ļ��Ϣ
	cout << "\nTrace complete.\n" << endl;

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


//����õ������ݱ�
BOOL DecodeIcmpResponse(char* pBuf, int iPacketSize, DECODE_RESULT& stDecodeResult)
{
	//������ݱ���С�ĺϷ���
	IP_HEADER* pIpHdr = (IP_HEADER*)pBuf;
	int iIpHdrLen = pIpHdr->hdr_len * 4;
	if (iPacketSize < (int)(iIpHdrLen+sizeof(ICMP_HEADER)))
		return FALSE;

	//����ICMP�����ͼ��id�ֶκ����к���ȷ���Ƿ��ǳ���Ӧ���յ�Icmp��
	ICMP_HEADER* pIcmpHdr = (ICMP_HEADER*)(pBuf+iIpHdrLen);
	USHORT usID, usSquNo;
	if (pIcmpHdr->type == ICMP_ECHO_REPLY)
	{
		usID = pIcmpHdr->id;
		usSquNo = pIcmpHdr->seq;
	}
	else if(pIcmpHdr->type == ICMP_TIMEOUT)
	{
		char* pInnerIpHdr = pBuf+iIpHdrLen+sizeof(ICMP_HEADER);		//�غ��е�IPͷ
		int iInnerIPHdrLen = ((IP_HEADER*)pInnerIpHdr)->hdr_len * 4;//�غ��е�IPͷ��
		ICMP_HEADER* pInnerIcmpHdr = (ICMP_HEADER*)(pInnerIpHdr+iInnerIPHdrLen);//�غ��е�ICMPͷ
		usID = pInnerIcmpHdr->id;
		usSquNo = pInnerIcmpHdr->seq;
	}
	else
		return FALSE;

	if (usID != (USHORT)GetCurrentProcessId() || usSquNo !=stDecodeResult.usSeqNo) 
		return FALSE;

	//������ȷ�յ���ICMP���ݱ�
	if (pIcmpHdr->type == ICMP_ECHO_REPLY ||
		pIcmpHdr->type == ICMP_TIMEOUT)
	{
		//���ؽ�����
		stDecodeResult.dwIPaddr.s_addr = pIpHdr->sourceIP;
		stDecodeResult.dwRoundTripTime = GetTickCount()-stDecodeResult.dwRoundTripTime;

		//��ӡ��Ļ��Ϣ
		if (stDecodeResult.dwRoundTripTime)
			cout << setw(6) << stDecodeResult.dwRoundTripTime << " ms" << flush;
		else
			cout << setw(6) << "<1" << " ms" << flush;

		return TRUE;
	}

	return FALSE;
}
