//��Settings�� Compiler settings��� Linker settings���C:\Program Files (x86)\CodeBlocks\MinGW\lib\libws2_32.a
#include<Winsock2.h>
#include<string>
#include<iostream>
#include<tchar.h>
using namespace std;


int GetLocalHostName(string& sHostName)
{
    /*�����ʼʱ��û����������䣬gethostname()�������ô���
    �����10093����ʾû�гɹ����� WSAStarup(),���Լ���������*/
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
    char szHostName[256];
    int nRetCode;
    nRetCode=gethostname(szHostName,sizeof(szHostName));
    if(nRetCode!=0)
    {
        sHostName=_T("Not available");//_T("")��һ����,��������������ĳ���֧��Unicode����,��ͷ�ļ�#include<tchar.h>��

        return WSAGetLastError();

    }
     sHostName=szHostName;
     cout<<"������Ϊ:"<<sHostName<<endl;
     return 0;
}
int main()
{
    string s;
    int n;
    n=GetLocalHostName(s);
    cout<<n;//���gethostame()�Ƿ���óɹ�����ȷ������0������ʾ�������������򷵻ش����
}
