#include <iostream>
#include "LibeventServer.h"

static const int s_nPort = 9999;
using namespace Network;
int main()
{
    LibeventServer::Instance().InitServer(s_nPort);

    printf("start server at port 9999 success\n");
    getchar();


    LibeventServer::Instance().Stop();


    system("pause");
    return 0;
}