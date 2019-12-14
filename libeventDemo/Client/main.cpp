#include <iostream>
#include "LibeventHelper.h"

static const int s_nPort = 9999;

int main()
{
    LibeventHelper myLibClient;
    myLibClient.Init(s_nPort);
    myLibClient.Start();

    myLibClient.Stop();

    printf("finished \n");


    system("pause");
    return 0;
}