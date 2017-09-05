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

	//初始化winsock2环境
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		cerr << "\nFailed to initialize the WinSock2 DLL\n"
			 << "error code: " << WSAGetLastError() << endl;
		return -1;
	}

	//将命令行参数转换为IP地址
	u_long ulDestIP = inet_addr(argv[1]);
	if (ulDestIP == INADDR_NONE)
	{
		//转换不成功时按域名解析
		hostent* pHostent = gethostbyname(argv[1]);
		if (pHostent)
		{
			ulDestIP = (*(in_addr*)pHostent->h_addr).s_addr;

			//输出屏幕信息
			cout << "\nPinging to " << argv[1] 
				 << " [" << inet_ntoa(*(in_addr*)(&ulDestIP)) << "]"
				 << " with " << DEF_ICMP_DATA_SIZE << " bytes of data:\n" << endl;
		}
		else //解析主机名失败
		{
			cerr << "\nCould not resolve the host name " << argv[1] << '\n'
				 << "error code: " << WSAGetLastError() << endl;
			WSACleanup();
			return -1;
		}
	}
	else
	{
		//输出屏幕信息
		cout << "\nPinging to " << argv[1] 
			 << " with " << DEF_ICMP_DATA_SIZE << " bytes of data:\n" << endl;
	}
	
	//填充目的Socket地址
	sockaddr_in destSockAddr;
	ZeroMemory(&destSockAddr, sizeof(sockaddr_in));
	destSockAddr.sin_family = AF_INET;
	destSockAddr.sin_addr.s_addr = ulDestIP;
	
	//使用ICMP协议创建Raw Socket
	SOCKET sockRaw = WSASocket(AF_INET, SOCK_RAW, IPPROTO_ICMP, NULL, 0, WSA_FLAG_OVERLAPPED);
	if (sockRaw == INVALID_SOCKET)
	{
		cerr << "\nFailed to create a raw socket\n"
			 << "error code: " << WSAGetLastError() << endl;
		WSACleanup();
		return -1;
	}
	//设置端口属性
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
	
	//填充icmp数据包各字段
	ICMP_HEADER *pIcmpHeader = (ICMP_HEADER *)IcmpSendBuf;
	pIcmpHeader->type = ICMP_ECHO_REQUEST;
	pIcmpHeader->code = 0;
	pIcmpHeader->id = (USHORT)GetCurrentProcessId();
	memset(IcmpSendBuf+sizeof(ICMP_HEADER),'E',DEF_ICMP_DATA_SIZE);
	
	//循环发送3个请求回显icmp数据包
	DECODE_RESULT stDecodeResult;
	for(int usSeqNo = 0;usSeqNo <= 3;usSeqNo++)
	{
		//以下seq和cksum两个参数是每次循环都需要改变的参数，所以放在循环内部每次循环都进行修改
		pIcmpHeader->seq = htons(usSeqNo);
		pIcmpHeader->cksum = 0;
		pIcmpHeader->cksum = GenerateChecksum((USHORT *)IcmpSendBuf,sizeof(ICMP_HEADER)+DEF_ICMP_DATA_SIZE);

		//记录序列号和当前时间
		stDecodeResult.usSeqNo = usSeqNo;
		stDecodeResult.dwRoundTripTime = GetTickCount();

		//发送ICMP的EchoRequest数据报
		if (sendto(sockRaw, IcmpSendBuf, sizeof(IcmpSendBuf), 0, 
				   (sockaddr*)&destSockAddr, sizeof(destSockAddr)) == SOCKET_ERROR)
		{
			//如果目的主机不可达则直接退出
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
			if(iReadLen != SOCKET_ERROR)  //正确接收到一个icmp数据包
			{
				//解码接收到的数据包并判断是否是返回给本进程的icmp回应数据包
				if(DecodeIcmpResponse(IcmpRecvBuf,iReadLen,stDecodeResult) && (stDecodeResult.dwIPaddr.s_addr == destSockAddr.sin_addr.s_addr))
					cout << "Reply from " << inet_ntoa(stDecodeResult.dwIPaddr) <<" : bytes=" << DEF_ICMP_DATA_SIZE 
						 << " RTT=" << stDecodeResult.dwRoundTripTime << "ms, TTL=" << stDecodeResult.iTTL << "." << endl;
				break;
				
			}
			else if(WSAGetLastError() == WSAETIMEDOUT) //接收超时
			{
				cout << "Request timed out." << endl;
				break;
			}
			else	//其他错误
			{
				cerr << "\nFailed to call recvfrom\n"
					 << "error code: " << WSAGetLastError() << endl;
				closesocket(sockRaw);
				WSACleanup();
				return -1;
			}
		}
	}

	//输出屏幕信息
	cout << "\nPing complete.\n" << endl;

	closesocket(sockRaw);
	WSACleanup();
	return 0;

}


//产生网际校验和
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
{	//获取收到ip数据包的首部信息
	IP_HEADER *pIpHrd = (IP_HEADER *)pBuf;
	int iIpHrdLen = pIpHrd->hdr_len * 4;
	if (iPacketSize < (int)(iIpHrdLen + sizeof(ICMP_HEADER)))
		return false;

	//指针指向icmp数据包的首地址
	ICMP_HEADER *pIcmpHrd = (ICMP_HEADER *)(pBuf+iIpHrdLen);
	
	USHORT usID,usSquNo;
	//获得的数据包的type字段为ICMP_ECHO_REPLY，即收到一个回显应答icmp数据包
	if (pIcmpHrd->type == ICMP_ECHO_REPLY)  
	{
		usID = pIcmpHrd->id;
		usSquNo = ntohs(pIcmpHrd->seq);  //接收到的是网络字节顺序的seq字段信息，需要转成主机字节顺序
	}

	if(usID != GetCurrentProcessId() || usSquNo != stDecodeResult.usSeqNo)
		return false;

	//记录对方主机的IP地址以及计算往返延时RTT
	if (pIcmpHrd->type == ICMP_ECHO_REPLY)
	{
		stDecodeResult.iTTL = pIpHrd->ttl;
		stDecodeResult.dwIPaddr.s_addr = pIpHrd->sourceIP;
		stDecodeResult.dwRoundTripTime = GetTickCount() - stDecodeResult.dwRoundTripTime;
		return true;
	}
	return false;
}