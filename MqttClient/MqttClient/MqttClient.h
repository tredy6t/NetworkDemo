#pragma once

#include "MQTTAsync.h"
#include "CommonDefines.h"
#include <map>
#include <mutex>
#include <vector>
#include <thread>

enum TOPIC_STATE
{
    NOT_SUBSCRIBE,
    SUBSCRIBE_SUCCESS,
    SUBSCRIBE_FAILED,
    SUBSCRIBE_REMOVE_SUCCESS,
    SUBSCRIBE_REMOVE_FAILED,
};
class CMqttClient
{
private:
    CMqttClient();
public:
    static CMqttClient& Instance();
    ~CMqttClient();
    bool Init(const BrokerInfo& infoBroker);
    bool IsConnected();
    void UpdateConnState(bool state);
    void ResetConnecting();
    void NotifyExit();
    //subscribe/unsubscribe topic
    bool SubscribeTopic(const std::string& strTopic);
    int GetSubscribedState(const std::string& strTopic);
    void UpdateSubscribeState(TOPIC_STATE state);
    bool UnsubscribeTopic(const std::string& strTopic);
    bool IsTopicSubscribed(const std::string& strTopic);
    void GetSubscribedTopics(std::vector<std::string>& topics);
    bool SubscribeTopics(const std::vector<std::string>& vecTopic);
    bool UnsubscribeTopics(const std::vector<std::string>& vecTopic);
    void ResubscribeTopics();
    //publish topic
    bool PublishTopic(const std::string& strTopic, const std::string& strMsg);

private:
    bool init_client();
    bool connect_broker();
    void reconnect_broker();
    void disconnect();
    bool check_can_subscribe();

private:
    bool m_bExit;
    BrokerInfo m_infoBroker;
    std::string m_strClientId;      //for now, it can be ip
    std::mutex m_mtConn;
    bool m_bConnected;      //connect broker state
    bool m_bIsConnecting;   //connecting
    std::thread m_threadReconn;

    std::mutex m_mtSubscribe;
    bool m_bSubscribing;    //subscribe topic

    std::mutex m_mtTopic;
    std::map<std::string, bool>m_mapTopic;
    std::vector<std::string>m_vecWaitForSubscrib;

    MQTTAsync m_clientMqtt;    //mqtt client

};

