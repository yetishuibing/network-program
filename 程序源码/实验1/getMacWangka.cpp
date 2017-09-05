#include<Winsock2.h>
#include<string>
#include<iostream>
#include<tchar.h>
#include<Nb30.h>
#include <windows.h>
 #include <stdlib.h>
 #include<stdio.h>
using namespace std;
struct ASTAT
    {
         ADAPTER_STATUS adapt;
         NAME_BUFFER NameBuff[30];
    } Adapter;
UCHAR GetAddressByIndex(int lana_num, ASTAT &Adapter)
 {
   NCB ncb;//网络控制块
   UCHAR uRetCode;
   memset(&ncb, 0, sizeof(ncb));
   //指定网卡号，首先对选定的网卡发送一个NCBRESET命令，以便进行初始化
   ncb.ncb_command = NCBRESET;
   ncb.ncb_lana_num = lana_num;
   uRetCode=Netbios(&ncb);//取得该网络控制块的指针
   memset(&ncb, 0, sizeof(ncb));
   ncb.ncb_command = NCBASTAT;
   ncb.ncb_lana_num = lana_num;//指定网卡号
   strcpy((char*)ncb.ncb_callname,"*        ");
   ncb.ncb_buffer=(unsigned char *)&Adapter;
   //指定返回的信息存放的变量
   ncb.ncb_length=sizeof(Adapter);
   //发送NCBASTAT命令以获取网卡的信息
   uRetCode=Netbios(&ncb);
   return uRetCode;
}
int main()
{

    NCB ncb;
    UCHAR uRetCode;
    int num=0;
    LANA_ENUM lana_enum;
    memset(&ncb,0,sizeof(ncb));
    ncb.ncb_command=NCBENUM;
    ncb.ncb_buffer=(unsigned char *)&lana_enum;
    ncb.ncb_length=sizeof(lana_enum);
    //向网卡发送NCBENUM命令，以获取当前机器的网卡信息，包括网卡的个数以及编号
    uRetCode=Netbios(&ncb);
    char acMAC[18];
    if(uRetCode==0)
    {
        num=lana_enum.length;
        for(int i=0;i<num;i++)
        {
            ASTAT Adapter;

            if(GetAddressByIndex(lana_enum.lana[i],Adapter)==0)
            {

               sprintf(acMAC, "%02X:%02X:%02X:%02X:%02X:%02X",int (Adapter.adapt.adapter_address[0]),
                        int (Adapter.adapt.adapter_address[1]),
                        int (Adapter.adapt.adapter_address[2]),
                        int (Adapter.adapt.adapter_address[3]),
                        int (Adapter.adapt.adapter_address[4]),
                        int (Adapter.adapt.adapter_address[5]));//以指定的格式将MAC地址写进acMAC中
                cout << "Adapter " << int (lana_enum.lana[i]) <<//获取网卡号，和MAC地址
                  "'s MAC is " << acMAC << endl;

           }
        }
    }


}
