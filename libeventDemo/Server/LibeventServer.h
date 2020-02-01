/**************************************************************
* @brief:       wrap of libevent server
* @auth:         Wite_Chen
* @date:         20200129
* @update:
* @copyright:
*
***************************************************************/
#pragma once
#include <iostream>
#include <string>
#include <mutex>
#include <map>
#include <thread>
#include <errno.h>
#include <event2/event.h>
#include <event2/listener.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <event2/thread.h>
#include <event2/util.h>
#ifdef _WIN32
#include <winsock2.h>
#include <windows.h> 
#include <ws2tcpip.h>
#endif // _Win32
#include <signal.h>


namespace Network
{
    class LibeventServer
    {
        // default cache size is 4096
        const static int kMaxData = 4096;
        LibeventServer();
    public:
        static LibeventServer& Instance();
        ~LibeventServer();
        bool InitServer(int port, int timeout = 60);
        void Stop();

    private:
        /************************************************************
        *                   libevent initialization                 *
        *************************************************************/
        static void conn_cb(evconnlistener *listener,
                                    evutil_socket_t fd,
                                    struct sockaddr *sock,
                                    int socklen, 
                                    void *arg);
        static void set_tcp_no_delay(evutil_socket_t fd);
        static void read_cb(bufferevent *bev, void *arg);
        static void write_cb(bufferevent *bev, void *arg);
        static void event_cb(bufferevent *bev, short events, void *arg);
        static void signal_cb(evutil_socket_t sig, short events, void *arg);
        static void timeout_cb(evutil_socket_t sig, short events, void *arg);

    private:
        /************************************************************
        *                   data interactive                        *
        *************************************************************/
        void run();
        void add_client(evutil_socket_t fd, bufferevent *pEvtBuffer);
        void remove_client(evutil_socket_t fd);

    public:
        std::thread m_thRun;
        event_base *m_pBase;
        evconnlistener *m_pListener;
        struct event* m_pEvtStop;
        struct event* m_pEvtTimeout;
        std::mutex m_mtLock;
        std::map<evutil_socket_t, bufferevent*> m_mapEvtBuffer;

    };
}
