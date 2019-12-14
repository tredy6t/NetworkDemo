#include <iostream>
#include "LibeventHelper.h"

static const int s_nPort = 9999;

int main()
{
    LibeventHelper myLibServer;
    myLibServer.Init(s_nPort);
    myLibServer.Start();
    myLibServer.Stop();


    system("pause");
    return 0;
}