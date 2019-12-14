#include <iostream>
#include "MqttClient.h"
#include <sstream>
#include <thread>
#include <vector>
#include <signal.h>
#include <windows.h>


BrokerInfo infoBroker;
std::vector<std::string>veccSubscribeTopic;
std::vector<std::string>vecPublishTopic;
bool is_exit = false;

void InitSubscribeTopic()
{
    std::string strTopicDetect = "/senscape/detect/#";
    std::string strTopicRequest = "111adsaaa";
    veccSubscribeTopic.push_back(strTopicDetect);
    veccSubscribeTopic.push_back(strTopicRequest);
}

void InitPublisTopic()
{
    std::string strMsgConf = "{\"action\":\"req_conf\",\"id\":\"111adsaaa\",\"seqno\":4}";
    std::string strMsgVersion = "{\"action\":\"req_version\",\"id\":\"111adsaaa\",\"seqno\":4}";
    vecPublishTopic.push_back(strMsgConf);
    vecPublishTopic.push_back(strMsgVersion);
}

void Init()
{
    //broker info
    infoBroker.strBrokerAddr = "192.168.2.22";
    infoBroker.nPort = 1883;
    infoBroker.strType = "tcp"; //ssl verify, the prefix is ssl, or tcp
    infoBroker.strUser = "admin";
    infoBroker.strPassword = "admin";
    std::stringstream ss;
    ss << infoBroker.strType << "://" << infoBroker.strBrokerAddr << ":" << infoBroker.nPort;
    infoBroker.strConnInfo = ss.str();

    TopicInfo infoTopic;
    ////topic info
    //infoTopic.nType = 0;
    //infoTopic.strClientId = "mqtt_sub";
    //
    //infoTopic.strRoot = "senscape";
    //infoTopic.strLocation = "detect";
    //infoTopic.strWideCard = "#";
    //if (0 == infoTopic.nType)
    //{
    //    std::stringstream ss;
    //    ss << "/" << infoTopic.strRoot;
    //    if (!infoTopic.strLocation.empty())
    //    {
    //        ss << "/" << infoTopic.strLocation;
    //    }
    //    if (!infoTopic.strDeviceId.empty())
    //    {
    //        ss << "/" << infoTopic.strDeviceId;
    //    }
    //    ss << "/" << infoTopic.strWideCard;
    //    infoTopic.strTopic = ss.str();
    //}
    //topic info, get personal topic
    //it should publish first according to the personal topic rule
    //and the can subscribe
    infoTopic.nType = 0;
    infoTopic.strClientId = "mqtt_sub";
    infoTopic.strDeviceId = "111adsaaa";    //publish client_id
    if (0 == infoTopic.nType)
    {
        std::stringstream ss;
        if (!infoTopic.strRoot.empty())
        {
            ss << infoTopic.strRoot;
        }
        if (!infoTopic.strDeviceId.empty())
        {
            ss << infoTopic.strDeviceId;
        }

        if (!infoTopic.strWideCard.empty())
        {
            ss << "/" << infoTopic.strWideCard;
        }
        infoTopic.strTopic = ss.str();
    }
    InitSubscribeTopic();
    InitPublisTopic();
}

void PrintSubscribeTopic()
{
    for (int i = 0; i < veccSubscribeTopic.size(); ++i)
    {
        std::cout << "Topic " << i << "<->" << veccSubscribeTopic[i] << std::endl;
    }
}

void PrintPublishTopic()
{
    for (int i = 0; i < vecPublishTopic.size(); ++i)
    {
        std::cout << "Topic " << i << "<->" << vecPublishTopic[i] << std::endl;
    }
}

BOOL ConsoleEventHandler(DWORD dwCtrlType)
{
    is_exit = true;
    CMqttClient::Instance().NotifyExit();
    switch (dwCtrlType)
    {
    case CTRL_C_EVENT:// handle the ctrl-c signal
    {
        printf("ctrl-c event\n\n");
        return FALSE;
    }
    case CTRL_CLOSE_EVENT:// ctrl-close: confirm that the user wants to exit.
    {
        printf("ctrl-close event\n\n");
        return FALSE;
    }
    case CTRL_BREAK_EVENT:// pass other signals to the next handler.
    {
        printf("ctrl-break event\n\n");
        return FALSE;
    }
    case CTRL_LOGOFF_EVENT:
    {
        printf("ctrl-logoff event\n\n");
        return FALSE;
    }
    case CTRL_SHUTDOWN_EVENT:
    {
        printf("ctrl-shutdown event\n\n");
        return FALSE;
    }
    default:
    {
        return FALSE;
    }
    }
}


int main()
{
    Init();
    if (!CMqttClient::Instance().Init(infoBroker))
    {
        std::cout << "Init subscribe-client failed\n";
        return -1;
    }
    SetConsoleCtrlHandler((PHANDLER_ROUTINE)ConsoleEventHandler, true);
    int nCheckTime = 20;
    int nCheck = 0;
    std::cout << "Check is connected broker...\n";
    while (!is_exit)
    {
        if (CMqttClient::Instance().IsConnected())
        {
            break;
        }
        if (++nCheck >= nCheckTime)
        {
            std::cout << "Connect broker time-out\n";
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
    std::cout << "Print key to continue:\n";
    std::cout << "q/Q to exit\n";
    std::cout << "s/S to subscribe one topic\n";
    std::cout << "u/U to subscribe one topic\n";
    std::cout << "p/P to publish one message\n";
    std::cout << "l/L to list subscribed topic\n";
    while (!is_exit)
    {
        char ch;
        std::cin >> ch;
        switch (ch)
        {
        case 's':
        case 'S':   //订阅单主题
        {
            PrintSubscribeTopic();
            std::cout << "Enter topic num:";
            int nTopic;
            std::cin >> nTopic;
            if (nTopic >= 0 && nTopic < veccSubscribeTopic.size())
            {
                std::cout << "Begin subscribe topic...\n";
                CMqttClient::Instance().SubscribeTopic(veccSubscribeTopic[nTopic]);
            }
            else
            {
                std::cout << "Invalid topic:" << nTopic << std::endl;
            }
            break;
        }
        case 'u':
        case 'U':   //取消订阅单主题
        {
            std::vector<std::string>vecTopic;
            CMqttClient::Instance().GetSubscribedTopics(vecTopic);
            std::cout << "Subscribed topics:\n";
            for (int i = 0; i < vecTopic.size(); ++i)
            {
                std::cout << "Topic " << i << ":" << vecTopic[i] << std::endl;
            }
            std::cout << "Enter topic num:";
            int nTopic;
            std::cin >> nTopic;
            if (nTopic >= 0 && nTopic < vecTopic.size())
            {
                std::cout << "Begin unsubscribe topic...\n";
                CMqttClient::Instance().UnsubscribeTopic(vecTopic[nTopic]);
            }
            break;
        }
        case 'm':
        case 'M':   //订阅多主题
        {
            std::cout << "Subscribe many topics";
            CMqttClient::Instance().SubscribeTopics(veccSubscribeTopic);
            break;
        }
        case 'n':
        case 'N':   //取消订阅多主题
        {
            std::cout << "Unsubscribe many topics";
            CMqttClient::Instance().UnsubscribeTopics(veccSubscribeTopic);
            break;
        }
        case 'l':
        case 'L':   //查看已订阅主题
        {
            std::vector<std::string>vecTopic;
            CMqttClient::Instance().GetSubscribedTopics(vecTopic);
            std::cout << "Current subscribed topics:\n";
            for (int i = 0; i < vecTopic.size(); ++i)
            {
                std::cout << "Topic " << i << ":" << vecTopic[i] << std::endl;
            }
            break;
        }
        case 'p':
        case 'P':   //发布主题
        {
            std::string strTopic = "/senscape/client_id/4e301daa6bc0bdf2";
            PrintPublishTopic();
            std::cout << "Enter topic num:";
            int nTopic;
            std::cin >> nTopic;
            if (nTopic >= 0 && nTopic < vecPublishTopic.size())
            {
                CMqttClient::Instance().PublishTopic(strTopic, vecPublishTopic[nTopic]);
            }
            break;
        }
        case 'q':
        case 'Q':   //退出
        {
            std::cout << "It will close in 1 second\n";
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            return 0;
        }
        default:
        {
            if ('0' > ch || '127' < 'ch')
            {
                is_exit = true;
            }
            else
            {
                std::cout << "Invalid input num\n";
            }
            break;
        }
        }
        if (!is_exit)
        {
            std::cin.clear();
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }
    }
    
    return 0;
}