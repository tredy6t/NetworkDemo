/**************************************************************
* @brief:       wrap of libevent
* @auth:         Wite_Chen
* @date:         20200129
* @update:
* @copyright:
*
***************************************************************/
#pragma once
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
    class LibeventClient
    {
        // default cache size is 4096
        const static int kMaxData = 4096;
    public:
        LibeventClient();
        ~LibeventClient();
        bool InitClient(const std::string& strIp, int port, int timeout = 60);
        void  Stop();
        bool WriteData(const std::string& strData);

    private:
        /************************************************************
        *                   libevent initialization                 *
        *************************************************************/
        static void set_tcp_no_delay(evutil_socket_t fd);
        static void server_msg_cb(bufferevent *bev, void *arg);
        static void write_cb(bufferevent *bev, void *arg);
        static void event_cb(bufferevent *bev, short events, void *arg);
        static void signal_cb(evutil_socket_t sig, short events, void *arg);
        static void timeout_cb(evutil_socket_t sig, short events, void *arg);

    private:
        /************************************************************
        *                   data interactive                        *
        *************************************************************/
        void run();

    public:
        std::thread m_thRun;
        event_base *m_pBase;
        struct event* m_pEvtStop;
        struct event* m_pEvtTimeout;
        struct bufferevent* m_pEvtBuffer;

    };
}
