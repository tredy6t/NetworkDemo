#ifndef YULIBEVENTCLIENT_H_H
#define YULIBEVENTCLIENT_H_H


/*
**@author: wite_chen
**@date:   20190830
**@brief:  The client of SimLibeventClient
*/

#include <stdio.h>
#include <string>
#include <errno.h>
#include <iostream>
using namespace std;

#include "event2/event.h"
#include "event2/listener.h"
#include "event2/bufferevent.h"
#include "event2/buffer.h"
#include "event2/thread.h"
#include "event2/util.h"

#include <winsock2.h>
#include <windows.h> 
#include <ws2tcpip.h>

class LibeventHelper
{
public:
	LibeventHelper();
	~LibeventHelper();

	void  Init(int port);
	void  Start();
	void  Stop();

private:
	static LibeventHelper *pThis; // 存储回调函数调用的对象

	int tcp_connect_server(const char* server_ip, int port);
	static void cmd_msg_cb(int fd, short events, void* arg);
	static void readcb(struct bufferevent* bev, void* arg);
	static void writecb(struct bufferevent* bev, void* arg);
	static void eventcb(struct bufferevent *bev, short event, void *arg);
	static void set_tcp_no_delay(evutil_socket_t fd);
	static void timeoutcb(evutil_socket_t fd, short events, void *arg);
    static void signal_cb(evutil_socket_t sig, short events, void *arg);

public:
	event_base *m_pBase;
	char* m_pszMsg;
	struct event *m_evtimeout;
	struct bufferevent** m_bevs;
	struct timeval m_timeout;

	static int m_siLtotal_bytes_read;
	static int m_siLtotal_messages_read;

};

#endif