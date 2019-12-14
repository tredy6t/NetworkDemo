#pragma once
#include <string>

//mqtt broker info
struct BrokerInfo
{
    std::string strType;
    std::string strBrokerAddr;
    int nPort;
    std::string strUser;
    std::string strPassword;
    std::string strConnInfo;
};

//mqtt topic
struct TopicInfo
{
    int nType;                  //0,subscribe£¬1¡¢p2p(subscribe client_id)
    std::string strClientId;    //current client_id
    std::string strTopic;       //topic
    std::string strRoot;        //root
    std::string strLocation;    //location
    std::string strDeviceId;    //destination client_id
    std::string strWideCard;    //wildcard
};