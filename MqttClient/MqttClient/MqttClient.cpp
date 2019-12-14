#include "MqttClient.h"
#include <sstream>
#include <thread>
#include <iostream>

#define BRIXBOT_QOS         2
#define BRIXBOT_SERVER_CA_FILE	"../ssl/ca.crt"
#define BRIXBOT_CLIENT_CRT_FILE	"../ssl/client.crt"
#define BRIXBOT_CLIENT_KEY_FILE	"../ssl/client.key"

/******************************************************************************************
*                               macro                                                     *
*******************************************************************************************/
#define VERIFY_RETURN_VOID(exp) \
    if(!exp) \
    { \
        return; \
    }
#define VERIFY_RETURN(exp, ret) \
    if(!exp) \
    { \
        return ret; \
    }


/******************************************************************************************
*                               mqtt callback                                             *
*******************************************************************************************/
void onConnected(void* context, MQTTAsync_successData* response)
{
    CMqttClient::Instance().ResetConnecting();
    std::cout << "Connected broker." << std::endl;
    CMqttClient::Instance().UpdateConnState(true);
    CMqttClient::Instance().ResubscribeTopics();
}

void onDisconnect(void* context, MQTTAsync_successData* response)
{
    printf("Successful disconnection\n");
}

void onSubscribeDone(void* context, MQTTAsync_successData* response)
{
    std::cout << "Subscribe succeeded" << std::endl;
    CMqttClient::Instance().UpdateSubscribeState(SUBSCRIBE_SUCCESS);
}

void onSubscribeFailed(void* context, MQTTAsync_failureData* response)
{
    std::cout << "Failed to subscribe message. Error code: "
        << (response ? response->code : 0) << std::endl;
    CMqttClient::Instance().UpdateSubscribeState(SUBSCRIBE_FAILED);
}

void onUnsubscribeDone(void* context, MQTTAsync_successData* response)
{
    std::cout << "unsubscribe succeeded" << std::endl;
    CMqttClient::Instance().UpdateSubscribeState(SUBSCRIBE_REMOVE_SUCCESS);
}

void onUnsubscribeFailed(void* context, MQTTAsync_failureData* response)
{
    std::cout << "Failed to unsubscribe message. Error code: "
        << (response ? response->code : 0) << std::endl;
    CMqttClient::Instance().UpdateSubscribeState(SUBSCRIBE_REMOVE_FAILED);
}

int message_arrived(void* context, char* topicName, int topicLen,
    MQTTAsync_message* msg)
{
    std::cout << "Received a new message from topic \"" << topicName << "\"" << std::endl;
    std::cout << "  message: ";
    std::string strMsg((const char *)msg->payload, msg->payloadlen);
    std::cout << strMsg << std::endl;

    MQTTAsync_freeMessage(&msg);
    MQTTAsync_free(topicName);

    return 1;
}

void onConnectFailure(void* context, MQTTAsync_failureData* response)
{
    std::cout << "Failed to connect broker!" << std::endl;
    CMqttClient::Instance().UpdateConnState(false);
}

void onConnectLost(void *context, char *cause)
{
    std::cout << "Broker connection lost!" << std::endl;
    CMqttClient::Instance().UpdateConnState(false);
}

void onDeliveryComplete(void* context, MQTTAsync_token token)
{
    std::cout << "Message with token " << token
        << " has been delivered.(from global callback)" << std::endl;
}

void onSendDone(void* context, MQTTAsync_successData* response)
{
    std::cout << "Message with token " << response->token
        << " has been delivered.(from response callback)" << std::endl;
}

void onSendFailed(void* context, MQTTAsync_failureData* response)
{
    std::cout << "Failed to send message with token " << response->token
        << ". Error code: " << response->code << std::endl;
}

/******************************************************************************************
*                               mqtt client                                               *
*******************************************************************************************/
CMqttClient& CMqttClient::Instance()
{
    static CMqttClient g_Instance;
    return g_Instance;
}

CMqttClient::CMqttClient()
    : m_clientMqtt(nullptr)
    , m_bExit(false)
    , m_bConnected(false)
    , m_bSubscribing(false)
    , m_bIsConnecting(false)
{
    m_strClientId = "mqtt_client";
}


CMqttClient::~CMqttClient()
{
    if (m_threadReconn.joinable())
    {
        m_threadReconn.join();
    }
    disconnect();
    MQTTAsync_destroy(&m_clientMqtt);
}

bool CMqttClient::Init(const BrokerInfo& infoBroker)
{
    m_infoBroker = infoBroker;
    VERIFY_RETURN(init_client(), false);

    m_threadReconn = std::thread(&CMqttClient::reconnect_broker, this);
    return true;
}

bool CMqttClient::IsConnected()
{
    VERIFY_RETURN(!m_bConnected, true);

    m_bConnected = MQTTAsync_isConnected(m_clientMqtt);
    return m_bConnected;
}

void CMqttClient::UpdateConnState(bool state)
{
    std::lock_guard<std::mutex>lock(m_mtConn);
    m_bConnected = state;
}

void CMqttClient::ResetConnecting()
{
    m_bIsConnecting = false;
}

void CMqttClient::NotifyExit()
{
    m_bExit = true;
}

bool CMqttClient::SubscribeTopic(const std::string& strTopic)
{
    VERIFY_RETURN(check_can_subscribe(), false);
    m_vecWaitForSubscrib.clear();
    m_vecWaitForSubscrib.push_back(strTopic);
    MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer;
    opts.onSuccess = onSubscribeDone;
    opts.onFailure = onSubscribeFailed;

    int rc = MQTTASYNC_SUCCESS;
    if ((rc = MQTTAsync_subscribe(m_clientMqtt, strTopic.c_str(), BRIXBOT_QOS, &opts))
        != MQTTASYNC_SUCCESS)
    {
        printf("Failed to start subscribe, return code %d\n", rc);
        return false;
    }
    return true;
}

int CMqttClient::GetSubscribedState(const std::string& strTopic)
{
    std::lock_guard<std::mutex>lock(m_mtTopic);
    auto itMatch = m_mapTopic.find(strTopic);
    if (itMatch == m_mapTopic.end())
    {
        return NOT_SUBSCRIBE;
    }
    return (itMatch->second) ? SUBSCRIBE_SUCCESS : SUBSCRIBE_FAILED;
}

void CMqttClient::UpdateSubscribeState(TOPIC_STATE state)
{
    switch (state)
    {
    case SUBSCRIBE_SUCCESS:
    {
        std::map<std::string, int>mapTopic;
        for (const auto& item: m_vecWaitForSubscrib)
        {
            m_mapTopic[item] = state;
        }  
        break;
    }
    case SUBSCRIBE_REMOVE_SUCCESS:
    {
        std::lock_guard<std::mutex>lock(m_mtTopic);
        for (const auto& item : m_vecWaitForSubscrib)
        {
            m_mapTopic.erase(item);
        }
        break;
    }
    case SUBSCRIBE_REMOVE_FAILED:
    {
        for (const auto& item : m_vecWaitForSubscrib)
        {
            std::cout << "Unsubscibed topic failed," << item;
        }
        break;
    }
    default:
        break;
    }

    m_bSubscribing = false;
}

bool CMqttClient::UnsubscribeTopic(const std::string& strTopic)
{
    VERIFY_RETURN(check_can_subscribe(), false);

    m_vecWaitForSubscrib.clear();
    m_vecWaitForSubscrib.push_back(strTopic);
    MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer;

    opts.onSuccess = onUnsubscribeDone;
    opts.onFailure = onUnsubscribeFailed;

    int rc;
    if ((rc = MQTTAsync_unsubscribe(m_clientMqtt, strTopic.c_str(), &opts))
        != MQTTASYNC_SUCCESS)
    {
        printf("Failed to start subscribe, return code %d\n", rc);
        return false;
    }
    return true;
}

bool CMqttClient::IsTopicSubscribed(const std::string& strTopic)
{
    std::lock_guard<std::mutex>lock(m_mtTopic);
    if (m_mapTopic.find(strTopic) == m_mapTopic.end())
    {
        return false;
    }
    return true;
}

void CMqttClient::GetSubscribedTopics(std::vector<std::string>& topics)
{
    std::map<std::string, bool>mapTopic;
    {
        std::lock_guard<std::mutex>lock(m_mtTopic);
        mapTopic = m_mapTopic;
    }
    for (const auto& item : mapTopic)
    {
        topics.push_back(item.first);
    }
}

bool CMqttClient::SubscribeTopics(const std::vector<std::string>& vecTopic)
{
    VERIFY_RETURN(check_can_subscribe(), false);

    m_vecWaitForSubscrib.clear();
    m_vecWaitForSubscrib = vecTopic;
    MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer;
    opts.onSuccess = onSubscribeDone;
    opts.onFailure = onSubscribeFailed;

    int nTopic = vecTopic.size();
    int* pQos = new int[nTopic];
    char **pTopics = (char **)malloc(sizeof(char *) * nTopic);
    for (int i = 0; i < nTopic; ++i)
    {
        pTopics[i] = (char *)malloc(sizeof(char) * vecTopic[i].length() + 1);
        pTopics[i] = (char*)vecTopic[i].c_str();
        pQos[i] = BRIXBOT_QOS;
    }

    int rc = MQTTASYNC_SUCCESS;
    if ((rc = MQTTAsync_subscribeMany(m_clientMqtt, nTopic, pTopics, pQos, &opts))
        != MQTTASYNC_SUCCESS)
    {
        printf("Failed to start subscribe, return code %d\n", rc);
        delete[]pTopics;
        pTopics = nullptr;
        delete[]pQos;
        pQos = nullptr;
        return false;
    }
    delete[]pTopics;
    pTopics = nullptr;
    delete[]pQos;
    pQos = nullptr;
    return true;
}

bool CMqttClient::UnsubscribeTopics(const std::vector<std::string>& vecTopic)
{
    VERIFY_RETURN(check_can_subscribe(), false);

    m_vecWaitForSubscrib.clear();
    m_vecWaitForSubscrib = vecTopic;
    MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer;

    opts.onSuccess = onUnsubscribeDone;
    opts.onFailure = onUnsubscribeFailed;

    int nTopic = vecTopic.size();
    char **pTopics = (char **)malloc(sizeof(char *) * nTopic);
    for (int i = 0; i < nTopic; ++i)
    {
        pTopics[i] = (char *)malloc(sizeof(char) * vecTopic[i].length() + 1);
        pTopics[i] = (char*)vecTopic[i].c_str();
    }
    int rc;
    if ((rc = MQTTAsync_unsubscribeMany(m_clientMqtt, nTopic, pTopics, &opts))
        != MQTTASYNC_SUCCESS)
    {
        printf("Failed to start subscribe, return code %d\n", rc);
        delete[]pTopics;
        pTopics = nullptr;
        return false;
    }
    delete[]pTopics;
    pTopics = nullptr;
    return true;
}

void CMqttClient::ResubscribeTopics()
{
    VERIFY_RETURN_VOID(!m_mapTopic.empty());

    std::vector<std::string>vecTopics;
    for (const auto& item : m_mapTopic)
        vecTopics.push_back(item.first);
    m_mapTopic.clear();
    SubscribeTopics(vecTopics);
}

bool CMqttClient::PublishTopic(const std::string& strTopic, const std::string& strMsg)
{
    if (!m_bConnected)
    {
        std::cout << "Broker not connected\n";
        return false;
    }
    MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer;
    MQTTAsync_message msg = MQTTAsync_message_initializer;

    opts.onSuccess = onSendDone;
    opts.onFailure = onSendFailed;

    msg.payload = (char*)strMsg.c_str();
    msg.payloadlen = strMsg.length();
    msg.qos = BRIXBOT_QOS;
    msg.retained = 0;

    std::cout << "Sending message..." << std::endl;

    int rc;
    if ((rc = MQTTAsync_sendMessage(m_clientMqtt, strTopic.c_str(), &msg, &opts))
        != MQTTASYNC_SUCCESS)
    {
        std::cout << "Failed to start sendMessage, return code " << rc << std::endl;
        return false;
    }
    return true;
}

bool CMqttClient::init_client()
{
    int ret = MQTTASYNC_SUCCESS;
    if ((ret = MQTTAsync_create(&m_clientMqtt, m_infoBroker.strConnInfo.c_str(), m_strClientId.c_str(),
        MQTTCLIENT_PERSISTENCE_NONE, NULL)) != MQTTASYNC_SUCCESS)
    {
        std::stringstream ss;
        ss << "Failed to create MQTT client, return code " << ret;
        printf("%s\n", ss.str());
        return false;
    }

    if ((ret = MQTTAsync_setCallbacks(m_clientMqtt, NULL, onConnectLost,
        message_arrived, onDeliveryComplete)) != MQTTASYNC_SUCCESS)
    {
        std::stringstream ss;
        ss << "Failed to set callbacks, return code " << ret << std::endl;
        printf("%s\n", ss.str());
        return false;
    }
    return true;
}

bool CMqttClient::connect_broker()
{
    MQTTAsync_SSLOptions sslOpts = MQTTAsync_SSLOptions_initializer;
    sslOpts.trustStore = BRIXBOT_SERVER_CA_FILE;
    sslOpts.keyStore = BRIXBOT_CLIENT_CRT_FILE;
    sslOpts.privateKey = BRIXBOT_CLIENT_KEY_FILE;
    sslOpts.verify = 1;

    MQTTAsync_connectOptions connOpts = MQTTAsync_connectOptions_initializer;
    connOpts.keepAliveInterval = 20;
    connOpts.cleansession = 1;
    connOpts.username = m_infoBroker.strUser.c_str();
    connOpts.password = m_infoBroker.strPassword.c_str();

    connOpts.onSuccess = onConnected;
    connOpts.onFailure = onConnectFailure;
    connOpts.context = (void *)m_clientMqtt;
    connOpts.ssl = &sslOpts;
    //reconnect
    connOpts.automaticReconnect = 1;
    connOpts.minRetryInterval = 3;

    m_bIsConnecting = true;
    int rc = MQTTASYNC_SUCCESS;
    std::cout << "Connecting " << m_infoBroker.strConnInfo << std::endl;
    if ((rc = MQTTAsync_connect(m_clientMqtt, &connOpts)) != MQTTASYNC_SUCCESS)
    {
        std::cout << "Failed to connect broker, it returns " << rc << std::endl;
        return false;
    }
    return true;
}

void CMqttClient::reconnect_broker()
{
    while (!m_bExit)
    {
        if (!m_bConnected)
        {
            if (nullptr != m_clientMqtt)
            {
                if (m_bIsConnecting)
                {
                    std::this_thread::sleep_for(std::chrono::milliseconds(500));
                }
                else
                {
                    disconnect();
                    connect_broker();
                }
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
}

void CMqttClient::disconnect()
{
    MQTTAsync_disconnectOptions disc_opts = MQTTAsync_disconnectOptions_initializer;
    disc_opts.onSuccess = onDisconnect;
    MQTTAsync_disconnect(&m_clientMqtt, &disc_opts);
}

bool CMqttClient::check_can_subscribe()
{
    if (!m_bConnected)
    {
        std::cout << "Broker not connected\n";
        return false;
    }
    {
        std::lock_guard<std::mutex>lock(m_mtSubscribe);
        if (m_bSubscribing)
        {
            std::cout << "there is one topic subscribing...";
            return false;
        }
        else
        {
            m_bSubscribing = true;
        }
    }
    return true;
}