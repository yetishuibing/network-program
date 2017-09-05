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
   NCB ncb;//������ƿ�
   UCHAR uRetCode;
   memset(&ncb, 0, sizeof(ncb));
   //ָ�������ţ����ȶ�ѡ������������һ��NCBRESET����Ա���г�ʼ��
   ncb.ncb_command = NCBRESET;
   ncb.ncb_lana_num = lana_num;
   uRetCode=Netbios(&ncb);//ȡ�ø�������ƿ��ָ��
   memset(&ncb, 0, sizeof(ncb));
   ncb.ncb_command = NCBASTAT;
   ncb.ncb_lana_num = lana_num;//ָ��������
   strcpy((char*)ncb.ncb_callname,"*        ");
   ncb.ncb_buffer=(unsigned char *)&Adapter;
   //ָ�����ص���Ϣ��ŵı���
   ncb.ncb_length=sizeof(Adapter);
   //����NCBASTAT�����Ի�ȡ��������Ϣ
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
    //����������NCBENUM����Ի�ȡ��ǰ������������Ϣ�����������ĸ����Լ����
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
                        int (Adapter.adapt.adapter_address[5]));//��ָ���ĸ�ʽ��MAC��ַд��acMAC��
                cout << "Adapter " << int (lana_enum.lana[i]) <<//��ȡ�����ţ���MAC��ַ
                  "'s MAC is " << acMAC << endl;

           }
        }
    }


}
