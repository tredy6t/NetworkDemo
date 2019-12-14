#include <iostream>
#include "OnvifHelper.h"

int main()
{
    OnvifHelper onvifClient;
    onvifClient.DiscoveryDevice();


    std::string strIP = "192.168.2.200";
    std::string strUser = "admin";
    std::string strPwd = "admin123";
    std::string strRtsp;
    onvifClient.GetCameraRtsp(strIP, strUser, strPwd, strRtsp);
    std::cout << strRtsp.c_str() << std::endl;

    return 0;
}
