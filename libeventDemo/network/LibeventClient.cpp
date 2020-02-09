#include "LibeventClient.h"


using namespace Network;
LibeventClient::LibeventClient()
    : m_pBase(nullptr)
    , m_pEvtStop(nullptr)
    , m_pEvtBuffer(nullptr)
{
}

LibeventClient::~LibeventClient()
{
    Stop();
}

/*
* @brief:       init the client
* @date:        20200129
* @update:
* @param[in]:   port, port to listen
* @param[in]:   timeout, time to lost interactive
* @return:      bool, success to return true, or false
*/
bool LibeventClient::InitClient(const std::string& strIp, int port, int timeout)
{
#ifdef _WIN32
    WSADATA wsaData;
    DWORD Ret;
    if ((Ret = WSAStartup(MAKEWORD(2, 2), &wsaData)) != 0)
    {
        printf("WSAStartup failed with error %d\n", Ret);
        return false;
    }
#endif // _WIN32
    evthread_use_windows_threads();
    m_pBase = event_base_new();
    if (nullptr == m_pBase)
    {
        printf("couldn't allocate event base!\n");
        return false;
    }
    // signal
    m_pEvtStop = evsignal_new(m_pBase, SIGINT, signal_cb, m_pBase);
    evsignal_add(m_pEvtStop, nullptr);
    // timeout
    struct timeval timeVar { timeout, 0 };
    m_pEvtTimeout = evtimer_new(m_pBase, timeout_cb, m_pBase);
    evtimer_add(m_pEvtTimeout, &timeVar);
    // address of server
    struct sockaddr_in addrSock;
    memset(&addrSock, 0, sizeof(struct sockaddr_in));
    addrSock.sin_family = AF_INET;
    addrSock.sin_port = htons(port);
    addrSock.sin_addr.s_addr = inet_addr(strIp.c_str());

    m_pEvtBuffer = bufferevent_socket_new(m_pBase, -1, BEV_OPT_CLOSE_ON_FREE| BEV_OPT_THREADSAFE);
    if (nullptr == m_pEvtBuffer) return false;
    bufferevent_setcb(m_pEvtBuffer, server_msg_cb, nullptr, event_cb, nullptr);
    bufferevent_enable(m_pEvtBuffer, EV_READ | EV_WRITE);

    if (bufferevent_socket_connect(m_pEvtBuffer, (struct sockaddr *)&addrSock,
        sizeof(addrSock)) < 0)
    {
        printf("Error starting connection!\n");
        bufferevent_free(m_pEvtBuffer);
        m_pEvtBuffer = nullptr;
        return false;
    }
    m_thRun = std::thread(std::bind(&LibeventClient::run, this));
    return true;
}

/*
* @brief:       stop to dispatch the message
* @date:        20200129
* @update:
*/
void LibeventClient::Stop()
{
    if (nullptr != m_pEvtStop)
    {
        event_free(m_pEvtStop);
        m_pEvtStop = nullptr;
    }
    if (m_pEvtBuffer)
    {
        bufferevent_free(m_pEvtBuffer);
        m_pEvtBuffer = nullptr;
    }
    if (nullptr != m_pBase)
    {
        event_base_loopexit(m_pBase, nullptr);
        event_base_free(m_pBase);
        m_pBase = nullptr;
    }
    if (m_thRun.joinable())
    {
        m_thRun.join();
    }
}

/*
* @brief:       write data
* @date:        20200129
* @update:
* @param[in]:   strData, data to write
* @param[in]:   arg, extra para
*/
bool LibeventClient::WriteData(const std::string& strData)
{
    if (nullptr == m_pEvtBuffer) return false;
    // 0 to return true, or false
    return 0 == bufferevent_write(m_pEvtBuffer, strData.c_str(), strData.length() + 1);
}

/*
* @brief:       handle reading of client
* @date:        20200129
* @update:
* @param[in]:   pEvtBuffer, the buffer of event
* @param[in]:   arg, extra para
*/
void LibeventClient::server_msg_cb(bufferevent *pEvtBuffer, void *arg)
{
    char msg[1024];

    size_t len = bufferevent_read(pEvtBuffer, msg, sizeof(msg));
    msg[len] = '\0';

    printf("recv %s from server\n", msg);
}

/*
* @brief:       handle writing of server
* @date:        20200129
* @update:
* @param[in]:   pEvtBuffer, the buffer of event
* @param[in]:   arg, extra para
*/
void LibeventClient::write_cb(bufferevent *pEvtBuffer, void *arg)
{
}

/*
* @brief:       catch the event of client
* @date:        20200129
* @update:
* @param[in]:   pEvtBuffer, the buffer of event
* @param[in]:   arg, extra para
*/
void LibeventClient::event_cb(bufferevent *bev, short events, void *arg)
{
    struct evbuffer *output = bufferevent_get_output(bev);
    size_t remain = evbuffer_get_length(output);

    if (events & BEV_EVENT_TIMEOUT)
        printf("Timed out\n");  //if bufferevent_set_timeouts() called.
    else if (events & BEV_EVENT_EOF)
        printf("connection closed, remain %d\n", remain);
    else if (events & BEV_EVENT_ERROR)
        printf("some other error, remain %d\n", remain);
    else if (events & BEV_EVENT_CONNECTED)
    {
        printf("connect server success\n");
        return;
    }
    // it'll close the socket and free the buffer-cache automaticaly
    bufferevent_free(bev);
    struct event *pEvt = (struct event*)arg;
    if (pEvt != nullptr)
    {
        event_free(pEvt);
        pEvt = nullptr;
    }
}

/*
* @brief:       catch the event of socket
* @date:        20200129
* @update:
* @param[in]:   fd, socket id
* @param[in]:   event, the of event of client
* @param[in]:   arg, extra para
*/
void LibeventClient::signal_cb(evutil_socket_t fd, short events, void *arg)
{
    struct event_base *base = (event_base *)arg;
    printf("exception: interrupt, stop now!\n");

    event_base_loopexit(base, NULL);
}

/*
* @brief:       catch the timeout of socket
* @date:        20200129
* @update:
* @param[in]:   fd, socket id
* @param[in]:   event, the of event of client
* @param[in]:   arg, extra para
*/
void LibeventClient::timeout_cb(evutil_socket_t sig, short events, void *arg)
{
    struct event_base *base = (event_base *)arg;
    printf("error: timeout\n");

    event_base_loopexit(base, NULL);
}

/*
* @brief:       set the block state of socket
* @date:        20190830
* @update:
* @param:       fd, scoket id
*/
void LibeventClient::set_tcp_no_delay(evutil_socket_t fd)
{
    int iOne = 1;
    setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (const char*)&iOne, sizeof iOne);
}

/*
* @brief:       start dispatch message
* @date:        20200129
* @update:
*/
void LibeventClient::run()
{
    event_base_dispatch(m_pBase);
}