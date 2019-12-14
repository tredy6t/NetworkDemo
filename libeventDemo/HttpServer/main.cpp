#include <iostream>
#include "HttpHelper.h"


int main()
{
    HttpHelper httpHelper;
    if (!httpHelper.Init()) {

        return -1;
    }
    httpHelper.Start();
    getchar();
    httpHelper.Stop();

    return 0;
}