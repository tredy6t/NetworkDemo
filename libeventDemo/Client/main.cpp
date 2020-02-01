#include <iostream>
#include "LibeventClient.h"

static const int s_nPort = 9999;
std::string s_strIp = "127.0.0.1";
using namespace Network;

int main()
{
    LibeventClient myLibClient;
    if (!myLibClient.InitClient(s_strIp, s_nPort))
    {
        printf("Init client failed \n");
        return -1;
    }
    printf("Press any key to send message \n");
    getchar();
    myLibClient.WriteData("111Hello World");

    printf("finished \n");


    system("pause");
    return 0;
}