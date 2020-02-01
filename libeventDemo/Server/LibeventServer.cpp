#include "LibeventServer.h"


using namespace Network;
LibeventServer::LibeventServer()
    : m_pBase(nullptr)
    , m_pListener(nullptr)
    , m_pEvtStop(nullptr)
{
}

LibeventServer& LibeventServer::Instance()
{
    static LibeventServer g_Instance;
    return g_Instance;
}

LibeventServer::~LibeventServer()
{
    Stop();
}

/*
* @brief:       init the server
* @date:        20200129
* @update:
* @param[in]:   port, port to listen
* @param[in]:   timeout, time to lost interactive
* @return:      bool, success to return true, or false
*/
bool LibeventServer::InitServer(int port, int timeout)
{
#ifdef _WIN32
    WSADATA wsaData;
    DWORD Ret;
    if ((Ret = WSAStartup(MAKEWORD(2, 2), &wsaData)) != 0)
    {
        printf("WSAStartup failed with error %d\n", Ret);
        return false;
    }
    // support for sending message by volunteered
    // also create the bufferevent with BEV_OPT_THREADSAFE
    evthread_use_windows_threads();
#endif // _WIN32
    m_pBase = event_base_new();
    if (nullptr == m_pBase)
    {
        printf("couldn't allocate event base!\n");
        return false;
    }
    // signal
    m_pEvtStop = evsignal_new(m_pBase, SIGINT, signal_cb, m_pBase);
    evsignal_add(m_pEvtStop, nullptr);
    //// timeout
    //struct timeval timeVar { timeout, 0 };
    //m_pEvtTimeout = evtimer_new(m_pBase, timeout_cb, m_pBase);
    //evtimer_add(m_pEvtTimeout, &timeVar);

    struct sockaddr_in addrSock;
    memset(&addrSock, 0, sizeof(struct sockaddr_in));
    addrSock.sin_family = AF_INET;
    addrSock.sin_port = htons(port);

    m_pListener = evconnlistener_new_bind(m_pBase,
        conn_cb,
        nullptr,
        LEV_OPT_REUSEABLE | LEV_OPT_CLOSE_ON_FREE,
        -1, (struct sockaddr*)&addrSock,
        sizeof(struct sockaddr_in));

    if (nullptr == m_pListener)
    {
        printf("couldn't create listener!\n");
        return false;
    }
    m_thRun = std::thread(std::bind(&LibeventServer::run, &LibeventServer::Instance()));
    return true;
}

/*
* @brief:       stop to dispatch the message
* @date:        20200129
* @update:
*/
void LibeventServer::Stop()
{
    if (nullptr != m_pListener)
    {
        evconnlistener_free(m_pListener);
        m_pListener = nullptr;
    }
    if (nullptr != m_pEvtStop)
    {
        event_free(m_pEvtStop);
        m_pEvtStop = nullptr;
    }
    if (m_pListener)
    {
        evconnlistener_free(m_pListener);
        m_pListener = nullptr;
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
* @brief:       handle connection of client
* @date:        20200129
* @update:
* @param[in]:   listener, the listen object of server
* @param[in]:   fd, the socket id
* @param[in]:   sock, the info of socket address
* @param[in]:   socklen,  the length of socket address
* @param[in]:   arg, extra para
*/
void LibeventServer::conn_cb(evconnlistener *listener,
    evutil_socket_t fd,
    struct sockaddr *sock,
    int socklen,
    void *arg)
{
    printf("We got a new connection! Set up a bufferevent for it. accept a client %d\n", fd);
    event_base *base = evconnlistener_get_base(listener);
    //  event_base *base = (event_base*)arg;
    //setup a new bufferevent for connection
    bufferevent *bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE | BEV_OPT_THREADSAFE);

    set_tcp_no_delay(fd);
    bufferevent_setcb(bev, read_cb, write_cb, event_cb, NULL);
    bufferevent_enable(bev, EV_READ | EV_WRITE);
    LibeventServer::Instance().add_client(fd, bev);
}

/*
* @brief:       handle reading of client
* @date:        20200129
* @update:
* @param[in]:   pEvtBuffer, the buffer of event
* @param[in]:   arg, extra para
*/
void LibeventServer::read_cb(bufferevent *pEvtBuffer, void *arg)
{
    char msg[kMaxData + 1] = { 0 };
    int iLen = 0;
    evutil_socket_t fd = bufferevent_getfd(pEvtBuffer);
    while (iLen = bufferevent_read(pEvtBuffer, msg, sizeof(msg) - 1), iLen > 0)
    {
        msg[iLen] = '\0';
        printf("fd=%u, read len = %d\t read msg: %s\n", fd, iLen, msg);
        /*std::string strMsg = "Got it";
        int iRst = bufferevent_write(pEvtBuffer, strMsg.c_str(), strMsg.length());
        if (-1 == iRst)
        {
        printf("[socket_write_cb]:error occur!\n");
        }*/
    }
    char reply[] = "I has read your data";
    bufferevent_write(pEvtBuffer, reply, strlen(reply));
}

/*
* @brief:       handle writing of server
* @date:        20200129
* @update:
* @param[in]:   pEvtBuffer, the buffer of event
* @param[in]:   arg, extra para
*/
void LibeventServer::write_cb(bufferevent *pEvtBuffer, void *arg)
{
    /*
    char reply[] = "[server: i'm server, send 1111]";
    printf("writecb: len = %d\n", 1 + strlen(reply));
    int iRst = bufferevent_write(bev, reply, 1 + strlen(reply));
    if (-1 == iRst)
    {
    printf("[socket_write_cb]:error occur!\n");
    }
    */
    printf("Trigger the write event\n");
}

/*
* @brief:       catch the event of client
* @date:        20200129
* @update:
* @param[in]:   pEvtBuffer, the buffer of event
* @param[in]:   arg, extra para
*/
void LibeventServer::event_cb(bufferevent *bev, short events, void *arg)
{
    struct evbuffer *output = bufferevent_get_output(bev);
    size_t remain = evbuffer_get_length(output);

    if (events & BEV_EVENT_TIMEOUT)
        printf("Timed out\n");  //if bufferevent_set_timeouts() called.
    else if (events & BEV_EVENT_EOF)
    {
        evutil_socket_t fd = bufferevent_getfd(bev);
        printf("connection closed, remain %d\n", remain);
        LibeventServer::Instance().remove_client(fd);
    }
        
    else if (events & BEV_EVENT_ERROR)
        printf("some other error, remain %d\n", remain);
    else if (events & BEV_EVENT_CONNECTED)
    {
        printf("connect server success\n");
        evutil_socket_t fd = bufferevent_getfd(bev);
        set_tcp_no_delay(fd);
    }
    else
        // it'll close the socket and free the buffer-cache automaticaly
        bufferevent_free(bev);
}

/*
* @brief:       catch the event of socket
* @date:        20200129
* @update:
* @param[in]:   fd, socket id
* @param[in]:   event, the of event of client
* @param[in]:   arg, extra para
*/
void LibeventServer::signal_cb(evutil_socket_t fd, short events, void *arg)
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
void LibeventServer::timeout_cb(evutil_socket_t sig, short events, void *arg)
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
void LibeventServer::set_tcp_no_delay(evutil_socket_t fd)
{
    int iOne = 1;
    setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (const char*)&iOne, sizeof iOne);
}

/*
* @brief:       start dispatch message
* @date:        20200129
* @update:
*/
void LibeventServer::run()
{
    event_base_dispatch(m_pBase);
}

/*
* @brief:       cache the new client
* @date:        20200129
* @update:
* @param[in]:   fd, socket id
* @param[in]:   pEvtBuffer, the buffer of event
*/
void LibeventServer::add_client(evutil_socket_t fd, bufferevent *pEvtBuffer)
{
    std::lock_guard<std::mutex> lock(m_mtLock);
    m_mapEvtBuffer[fd] = pEvtBuffer;
}

/*
* @brief:       remove the client
* @date:        20200129
* @update:
* @param[in]:   fd, socket id
*/
void LibeventServer::remove_client(evutil_socket_t fd)
{
    std::lock_guard<std::mutex> lock(m_mtLock);
    m_mapEvtBuffer.erase(fd);
}
