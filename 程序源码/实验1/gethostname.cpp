//在Settings的 Compiler settings里的 Linker settings里加C:\Program Files (x86)\CodeBlocks\MinGW\lib\libws2_32.a
#include<Winsock2.h>
#include<string>
#include<iostream>
#include<tchar.h>
using namespace std;


int GetLocalHostName(string& sHostName)
{
    /*程序初始时，没有这两条语句，gethostname()函数调用错误，
    错误号10093，表示没有成功调用 WSAStarup(),所以加上这两句*/
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
    char szHostName[256];
    int nRetCode;
    nRetCode=gethostname(szHostName,sizeof(szHostName));
    if(nRetCode!=0)
    {
        sHostName=_T("Not available");//_T("")是一个宏,他的作用是让你的程序支持Unicode编码,在头文件#include<tchar.h>里

        return WSAGetLastError();

    }
     sHostName=szHostName;
     cout<<"主机名为:"<<sHostName<<endl;
     return 0;
}
int main()
{
    string s;
    int n;
    n=GetLocalHostName(s);
    cout<<n;//检查gethostame()是否调用成功，正确：返回0，并显示主机名，错误：则返回错误号
}
